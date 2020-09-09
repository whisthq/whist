import random
import string
import paramiko
import io
import time
import json
from pprint import pprint

import boto3
import botocore.exceptions


def check_str_param(val, name):
    if isinstance(val, str):
        return val
    raise Exception("Value {} was supposed to be a string, but was a {}".format(name, type(val)))


def check_bool_param(val, name):
    if isinstance(val, bool):
        return val
    raise Exception("Value {} was supposed to be a boolean, but was a {}".format(name, type(val)))


class ECSClient:
    """
    This class governs everything you need to generate, run, and track a specific task on ECS
    Args:
        region_name (Optional[str]):  which AWS region you're running on
        key_id (Optional[str]): the AWS access key ID to use
        access_key (Optional[str]): the AWS access key going with that key_id
        launch_type (Optional[str]): whether to use EC2 or FARGATE for the task
        base_cluster (Optional[str]): what cluster to run on, used for testing/mocking
        grab_logs (Optional[bool]):  whether to pull down ECS logs
        starter_client (boto3.client): starter ecs client, used for mocking
        starter_log_client (boto3.client): starter log client, used for mocking
        starter_ec2_client (boto3.client): starter log client, used for mocking
    TODO: find ECS specific logs, if they exist -- right now logs are stdout and stderr
    """

    def __init__(
        self,
        region_name="us-east-1",
        key_id="",
        access_key="",
        launch_type="EC2",
        base_cluster=None,
        starter_client=None,
        starter_log_client=None,
        starter_ec2_client=None,
        starter_s3_client=None,
        starter_ssm_client=None,
        starter_iam_client=None,
        starter_auto_scaling_client=None,
        grab_logs=True,
    ):
        self.key_id = check_str_param(key_id, "key_id")
        self.region_name = check_str_param(region_name, "region_name")
        self.access_key = check_str_param(access_key, "access_key")
        self.launch_type = check_str_param(launch_type, "launch_type")
        self.account_id = "123412341234"
        self.grab_logs = check_bool_param(grab_logs, "grab_logs")
        self.task_definition_arn = None
        self.tasks = []
        self.task_ips = {}
        self.tasks_done = []
        self.offset = -1
        self.container_name = None
        self.ecs_logs = dict()
        self.logs_messages = dict()
        self.warnings = []
        self.cluster = None
        #self.ec2 = boto3.resource('ec2', self.region_name)
        if starter_client is None:
            self.ecs_client = self.make_client("ecs")
            self.account_id = boto3.client("sts").get_caller_identity().get("Account")
        else:
            self.ecs_client = starter_client
        if starter_log_client is None:
            self.log_client = self.make_client("logs")
        else:
            self.log_client = starter_log_client
        if starter_ec2_client is None:
            self.ec2_client = self.make_client("ec2")
        else:
            self.ec2_client = starter_ec2_client
        if starter_s3_client is None:
            self.s3_client = self.make_client("s3")
        else:
            self.s3_client = starter_s3_client
        if starter_ssm_client is None:
            self.ssm_client = self.make_client("ssm")
        else:
            self.ssm_client = starter_ssm_client
        if starter_iam_client is None:
            self.iam_client = self.make_client("iam")
        else:
            self.iam_client = starter_iam_client
        if starter_auto_scaling_client is None:
            self.auto_scaling_client = self.make_client("autoscaling")
        else:
            self.auto_scaling_client = starter_auto_scaling_client
        self.set_cluster(cluster_name=base_cluster)

        # Create role + instance profile that allows containers to use SSM, S3, and EC2
        self.role_name = self.generate_name('role_name')
        assume_role_policy_document = {
            "Version": "2012-10-17", 
            "Statement": [
                {
                    "Effect": "Allow",
                    "Principal": {
                        "Service": [
                            "ec2.amazonaws.com"
                        ]
                    },
                    "Action": [
                        "sts:AssumeRole"
                    ]
                }
            ]
        }
        self.iam_client.create_role(
            RoleName=self.role_name,
            AssumeRolePolicyDocument=json.dumps(assume_role_policy_document),
        )
        self.iam_client.attach_role_policy(
            PolicyArn='arn:aws:iam::aws:policy/AmazonSSMManagedInstanceCore',
            RoleName=self.role_name,
        )
        self.iam_client.attach_role_policy(
            PolicyArn='arn:aws:iam::aws:policy/AmazonS3FullAccess',
            RoleName=self.role_name,
        )
        self.iam_client.attach_role_policy(
            PolicyArn='arn:aws:iam::aws:policy/service-role/AmazonEC2ContainerServiceforEC2Role',
            RoleName=self.role_name,
        )
        self.instance_profile = self.generate_name('instance_profile')
        self.iam_client.create_instance_profile(InstanceProfileName=self.instance_profile)
        self.iam_client.add_role_to_instance_profile(
            InstanceProfileName=self.instance_profile,
            RoleName=self.role_name
        )

    def make_client(self, client_type, **kwargs):
        """
        constructs an ECS client object with the given params
        Args:
            client_type (str): which ECS client you're trying to produce
            **kwargs: any add'l keyword params
        Returns:
            client: the constructed client object
        """
        clients = boto3.client(
            client_type,
            aws_access_key_id=(self.key_id if len(self.key_id) > 0 else None),
            aws_secret_access_key=(self.access_key if len(self.access_key) > 0 else None),
            region_name=(self.region_name if len(self.region_name) > 0 else None),
            **kwargs
        )
        return clients

    def generate_name(self, starter_name=''):
        letters = string.ascii_lowercase
        return starter_name + '_' + ''.join(random.choice(letters) for i in range(10))

    def create_cluster(self, capacity_providers, cluster_name=None):
        """
        Creates a new cluster with the specified capacity providers and sets the task's compute cluster to it
        """
        if isinstance(capacity_providers, str):
            capacity_providers = [capacity_providers]
        if not isinstance(capacity_providers, list):
            raise Exception("capacity_providers must be a list of strs")
        cluster_name = cluster_name or self.generate_name("cluster")
        self.ecs_client.create_cluster(
            clusterName=cluster_name, 
            capacityProviders=capacity_providers,
            defaultCapacityProviderStrategy=[
                {
                    'capacityProvider': capacity_provider,
                    'weight': 1,
                    'base': 0,
                } 
                for capacity_provider in capacity_providers
            ],
        )
        self.set_cluster(cluster_name)
        return cluster_name

    def set_cluster(self, cluster_name=None):
        """
        sets the task's compute cluster to be the first available/default compute cluster.
        """
        self.cluster = (
            check_str_param(cluster_name, 'cluster_name')
            if cluster_name
            else self.ecs_client.list_clusters()["clusterArns"][0]
        )

    def get_all_clusters(self):
        """
        returns list of all cluster ARNs
        """
        clusters, next_token = [], None
        while True:
            clusters_response = self.ecs_client.list_clusters(next_token) if next_token else self.ecs_client.list_clusters()
            clusters.extend(clusters_response['clusterArns'])
            next_token = clusters_response['nextToken'] if 'nextToken' in clusters_response else None
            if not next_token:
                break
        return clusters
    
    def get_containers_in_cluster(self, cluster):
        """
        returns list of all container instance IDs in the auto scaling group of the capacity provider for the cluster
        """
        instances = []
        capacity_providers = self.ecs_client.describe_clusters(clusters=[cluster])['clusters'][0]['capacityProviders']
        capacity_providers_info = self.ecs_client.describe_capacity_providers(capacityProviders=capacity_providers)['capacityProviders']
        auto_scaling_groups = list(map(lambda cp: cp['autoScalingGroupProvider']['autoScalingGroupArn'].split('/')[-1], capacity_providers_info))
        auto_scaling_groups_info = self.auto_scaling_client.describe_auto_scaling_groups(AutoScalingGroupNames=auto_scaling_groups)['AutoScalingGroups']
        for auto_scaling_group in auto_scaling_groups_info:
            pprint(auto_scaling_group)
            instances += list(map(lambda instance: instance['InstanceId'], auto_scaling_group['Instances']))
        print(capacity_providers, auto_scaling_groups, instances)
        return instances

    def terminate_containers_in_cluster(self, cluster):
        containers = self.get_containers_in_cluster(cluster)
        if containers:
            self.ec2_client.terminate_instances(InstanceIds=containers)

    def exec_commands_on_containers(self, containers, commands):
        resp = self.ssm_client.send_command(
            DocumentName="AWS-RunShellScript", # One of AWS' preconfigured documents
            Parameters={'commands': commands},
            InstanceIds=containers,
            OutputS3Region=self.region_name,
            OutputS3BucketName='fractal-container-outputs',
        )
        return resp

    def exec_commands_all_containers(self, commands):
        all_containers = []
        clusters = self.get_all_clusters()
        for cluster in clusters:
            all_containers.extend(self.get_containers_in_cluster(cluster))
        self.exec_commands_on_containers(all_containers, ssh_command)

    def get_clusters_usage(self, clusters=None):
        """
        gets usage of all clusters
        """
        clusters, clusters_usage = self.get_all_clusters(), {}
        for cluster in clusters:
            cluster_info = self.ecs_client.describe_clusters(clusters=[cluster], include=['STATISTICS'])['clusters'][0]
            containers, containers_usage = self.get_containers_in_cluster(cluster), {}
            for container in containers:
                instance_info = self.ecs_client.describe_container_instances(cluster=cluster, containerInstances=[container])['containerInstances'][0]
                resources = {}
                for resource in instance_info['remainingResources']:
                    if resource['name'] == 'CPU' or resource['name'] == 'MEMORY':
                        resources[resource['name']] = resource['integerValue']
                
                containers_usage[container] = {  
                    'remainingResources': resources,
                    'pendingTasksCount': instance_info['pendingTasksCount'],
                    'runningTasksCount': instance_info['runningTasksCount']
                }
            
            clusters_usage[cluster_info['clusterName']] = {
                'status': cluster_info['status'],
                'pendingTasksCount': cluster_info['pendingTasksCount'],
                'runningTasksCount': cluster_info['runningTasksCount'],
                'detailedStatistics': {stat['name']: stat['value'] for stat in cluster_info['statistics'] if 'Fargate' not in stat['name']},
                'registeredContainerInstancesCount': cluster_info['registeredContainerInstancesCount'],
                'containersUsage': containers_usage,
            }

        pprint(clusters_usage)
        return clusters_usage

    def set_and_register_task(
        self,
        command=None,
        entrypoint=None,
        basedict=None,
        port_mappings=None,
        family="echostart",
        containername="basictest",
        imagename=None,
        memory="512",
        cpu="256",
    ):
        """
        Generates a task corresponding to a given docker image, command, and entry point
        while setting instance attrs accordingly (logging and task attrs).
        Args:
            command (List[str]): Command to run on container
            entrypoint (List[str]): Entrypoint for container
            basedict (Optional[Dict[str, Any]]): the base parametrization of your task.
            port_mappings (Optional[List[Dict]): any port mappings you want on the host container, defaults to 8080 tcp.
            family (Optional[str]): what task family you want this task revising
            containername (Optional[str]):  what you want the container the task is on to be called
            imagename (Optional[str]): the URI for the docker image
            memory (Optional[str]): how much memory (in MB) the task needs
            cpu (Optional[str]): how much CPU (in vCPU) the task needs
        """
        fmtstr = family + str(self.offset + 1)
        if port_mappings is None:
            port_mappings = [{"hostPort": 8080, "protocol": "tcp", "containerPort": 8080}]
        base_log_config = {
            "logDriver": "awslogs",
            "options": {
                "awslogs-group": "/ecs/{}".format(fmtstr),
                "awslogs-region": self.region_name,
                "awslogs-stream-prefix": "ecs",
            },
        }
        try:
            self.log_client.create_log_group(logGroupName="/ecs/{}".format(fmtstr))
        except botocore.exceptions.ClientError as e:
            self.warnings.append(str(e))
        # TODO:  refactor this horrifying mess into multiple functions
        if basedict is None:
            basedict = {
                "executionRoleArn": "arn:aws:iam::{}:role/ecsTaskExecutionRole".format(
                    self.account_id
                ),
                "containerDefinitions": [
                    {
                        "logConfiguration": base_log_config,
                        "entryPoint": entrypoint,
                        "portMappings": port_mappings,
                        "command": command,
                        "cpu": 0,
                        "environment": [{"name": "TEST", "value": "end"}],
                        "image": (imagename if imagename is not None else "httpd:2.4"),
                        "name": containername,
                    }
                ],
                "placementConstraints": [],
                "memory": memory,
                "family": family,
                #"networkMode": "awsvpc",
                "cpu": cpu,
            }
            if basedict["containerDefinitions"][0]["command"] is None:
                basedict["containerDefinitions"][0].pop("command")
            if basedict["containerDefinitions"][0]["entryPoint"] is None:
                basedict["containerDefinitions"][0].pop("entryPoint")
        else:
            container_params = basedict["containerDefinitions"][0]
            if command is not None:
                container_params["command"] = command
            if entrypoint is not None:
                container_params["entryPoint"] = entrypoint
            if imagename is not None:
                container_params["image"] = imagename
            container_params["name"] = containername
            container_params["portMappings"] = port_mappings
            container_params["logConfiguration"] = base_log_config
            basedict["family"] = family

        response = self.ecs_client.register_task_definition(**basedict)
        arn = response["taskDefinition"]["taskDefinitionArn"]
        self.task_definition_arn = arn
        self.container_name = containername

    def add_task(self, task_arn):
        """
        Adds a task by arn to this client's task list.  Useful for keeping an eye on already running tasks.
        Args:
            task_arn: the ID of the task to add.


        """
        self.tasks.append(task_arn)
        self.tasks_done.append(False)
        self.offset += 1

    def run_task(self, use_launch_type=True, **kwargs):
        """
        sets this client's task running.
        TODO: explicitly add overrides as params here for cpu, command, and environment vars
        Args:
            **kwargs: Any add'l params you want the task to have
        """
        task_args = {
            'taskDefinition': self.task_definition_arn,
            'cluster': self.cluster,
        }
        if use_launch_type:
            task_args['launchType'] = self.launch_type
        taskdict = self.ecs_client.run_task(**task_args, **kwargs)
        task = taskdict["tasks"][0]
        running_task_arn = task["taskArn"]
        self.tasks.append(running_task_arn)
        self.tasks_done.append(False)
        self.offset += 1

    def stop_task(self, reason="user stopped", offset=0):
        self.ecs_client.stop_task(
            cluster=self.cluster, task=(self.tasks[offset]), reason=reason,
        )

    def create_launch_configuration(self, instance_type, ami, launch_config_name=None, cluster_name=None):
        if not cluster_name:
            cluster_name = self.generate_name('cluster')

        # Initial data/scripts to be run on all container instances
        userdata = """
            #!/bin/bash
            # Install Docker
            apt-get update -y && apt-get install -y docker.io
            echo iptables-persistent iptables-persistent/autosave_v4 boolean true | debconf-set-selections
            apt-get update -y && apt-get install -y docker.io
            echo iptables-persistent iptables-persistent/autosave_v6 boolean true | debconf-set-selections
            apt-get -y install iptables-persistent

            # Set iptables rules
            echo 'net.ipv4.conf.all.route_localnet = 1' >> /etc/sysctl.conf
            sysctl -p /etc/sysctl.conf
            iptables -t nat -A PREROUTING -p tcp -d 169.254.170.2 --dport 80 -j DNAT --to-destination 127.0.0.1:51679
            iptables -t nat -A OUTPUT -d 169.254.170.2 -p tcp -m tcp --dport 80 -j REDIRECT --to-ports 51679

            # Write iptables rules to persist after reboot
            iptables-save > /etc/iptables/rules.v4

            # Create directories for ECS agent
            mkdir -p /var/log/ecs /var/lib/ecs/data /etc/ecs

            # Write ECS config file
            cat << EOF > /etc/ecs/ecs.config
            ECS_DATADIR=/data
            ECS_ENABLE_TASK_IAM_ROLE=true
            ECS_ENABLE_TASK_IAM_ROLE_NETWORK_HOST=true
            ECS_LOGFILE=/log/ecs-agent.log
            ECS_AVAILABLE_LOGGING_DRIVERS=["json-file","awslogs"]
            ECS_LOGLEVEL=info
            ECS_CLUSTER={}
            EOF

            # Write systemd unit file
            cat << EOF > /etc/systemd/system/docker-container@ecs-agent.service
            [Unit]
            Description=Docker Container %I
            Requires=docker.service
            After=docker.service

            [Service]
            Restart=always
            ExecStartPre=-/usr/bin/docker rm -f %i 
            ExecStart=/usr/bin/docker run --name %i \
            --restart=on-failure:10 \
            --volume=/var/run:/var/run \
            --volume=/var/log/ecs/:/log \
            --volume=/var/lib/ecs/data:/data \
            --volume=/etc/ecs:/etc/ecs \
            --net=host \
            --env-file=/etc/ecs/ecs.config \
            amazon/amazon-ecs-agent:latest
            ExecStop=/usr/bin/docker stop %i

            [Install]
            WantedBy=default.target
            EOF

            systemctl enable docker-container@ecs-agent.service
            systemctl start docker-container@ecs-agent.service
        """.format(cluster_name)

        pprint(self.iam_client.list_instance_profiles())
        launch_config_name = launch_config_name or self.generate_name('launch_configuration')
        response = self.auto_scaling_client.create_launch_configuration(
            LaunchConfigurationName=launch_config_name,
            ImageId=ami,
            InstanceType=instance_type,
            IamInstanceProfile=self.instance_profile,
            UserData=userdata,
        )
        return launch_config_name

    def create_auto_scaling_group(
        self,
        launch_config_name,
        auto_scaling_group_name=None,
        min_size=1,
        max_size=10,
        availability_zones=None,
    ):
        availability_zones = availability_zones or [self.region_name + 'a']
        if isinstance(availability_zones, str):
            availability_zones = [availability_zones]
        if not isinstance(availability_zones, list):
            raise Exception("availability_zones should be a list of strs")
        auto_scaling_group_name = auto_scaling_group_name or self.generate_name(
            'auto_scaling_group'
        )
        response = self.auto_scaling_client.create_auto_scaling_group(
            AutoScalingGroupName=auto_scaling_group_name,
            LaunchConfigurationName=launch_config_name,
            MaxSize=max_size,
            MinSize=min_size,
            AvailabilityZones=availability_zones,
        )
        return auto_scaling_group_name

    def create_capacity_provider(self, auto_scaling_group_name, capacity_provider_name=None):
        auto_scaling_group_info = self.auto_scaling_client.describe_auto_scaling_groups(
            AutoScalingGroupNames=[auto_scaling_group_name]
        )
        auto_scaling_group_arn = auto_scaling_group_info['AutoScalingGroups'][0][
            'AutoScalingGroupARN'
        ]
        capacity_provider_name = capacity_provider_name or self.generate_name('capacity_provider')
        response = self.ecs_client.create_capacity_provider(
            name=capacity_provider_name,
            autoScalingGroupProvider={
                'autoScalingGroupArn': auto_scaling_group_arn,
                'managedScaling': {
                    'maximumScalingStepSize': 1000,
                    'minimumScalingStepSize': 1,
                    'status': 'ENABLED',
                    'targetCapacity': 100,
                },
            },
        )
        return capacity_provider_name

    def get_instance_ip(self, offset=0):
        if self.tasks_done[offset]:
            return False
        response = self.ecs_client.describe_tasks(tasks=self.tasks, cluster=self.cluster)
        resp = response["tasks"][offset]
        if resp["lastStatus"] == "RUNNING" or resp["lastStatus"] == "STOPPED":
            container_info = self.ecs_client.describe_container_instances(
                cluster=self.cluster, containerInstances=[resp['containerInstanceArn']]
            )
            ec2_id = container_info['containerInstances'][0]['ec2InstanceId']
            ec2_info = self.ec2_client.describe_instances(InstanceIds=[ec2_id])
            public_ip = ec2_info['Reservations'][0]['Instances'][0].get('PublicIpAddress', -1)
            self.task_ips[offset] = public_ip
            return True
        return False

    def check_if_done(self, offset=0):
        """
        polls ECS to see if the task is complete, pulling down logs if so
        TODO:  handle pagination for 100+ tasks
        Args:
             offset (int): which task to check for
        Returns:
             boolean corresponding to completion
        """
        if self.tasks_done[offset]:
            return True
        response = self.ecs_client.describe_tasks(tasks=self.tasks, cluster=self.cluster)
        resp = response["tasks"][offset]
        if resp["lastStatus"] == "STOPPED":
            if self.grab_logs:
                check_arn = resp["taskDefinitionArn"]
                taskDefinition = self.ecs_client.describe_task_definition(taskDefinition=check_arn)[
                    "taskDefinition"
                ]
                logconfig = taskDefinition["containerDefinitions"][0]["logConfiguration"]["options"]
                streamobj = self.log_client.describe_log_streams(
                    logGroupName=logconfig["awslogs-group"],
                    logStreamNamePrefix=logconfig["awslogs-stream-prefix"],
                )
                logs = self.log_client.get_log_events(
                    logGroupName=logconfig["awslogs-group"],
                    logStreamName=streamobj["logStreams"][-1]["logStreamName"],
                )
                self.ecs_logs[offset] = logs["events"]
                self.logs_messages[offset] = [log["message"] for log in logs["events"]]
            self.tasks_done[offset] = True
            return True
        return False

    def spin_til_running(self, offset=0, time_delay=5):
        while not self.get_instance_ip(offset=offset):
            time.sleep(time_delay)
    
    def spin_til_containers_up(self, cluster_name, time_delay=5):
        container_instances = []
        while not container_instances:
            container_instances = testclient.get_containers_in_cluster(cluster_name)
            time.sleep(time_delay)
        
        # wait another 30 seconds just to be safe
        time.sleep(30)
        return container_instances

    def spin_til_done(self, offset=0, time_delay=5):
        """
        spinpolls the ECS servers to test if the task is done
        Args:
            offset (int): the offset of the task within the task array
            time_delay (int): how long to wait between polls, seconds
        """
        while not self.check_if_done(offset=offset):
            time.sleep(time_delay)

    def spin_all(self, time_delay=5):
        """
        spinpolls all generated tasks
        Args:
            time_delay (int): how long to wait between polls, seconds
        """
        for i in range(self.offset + 1):
            self.spin_til_done(offset=i, time_delay=time_delay)

    def run_all(self, time_delay=5):
        for i in range(self.offset + 1):
            self.spin_til_running(offset=i, time_delay=time_delay)

    def create_auto_scaling_cluster(self, instance_type='t2.small', ami='ami-04cfcf6827bb29439'):
        cluster_name = self.generate_name('cluster')
        launch_config_name = testclient.create_launch_configuration(instance_type=instance_type, ami=ami, cluster_name=cluster_name, launch_config_name=None)
        auto_scaling_group_name = testclient.create_auto_scaling_group(launch_config_name=launch_config_name)
        capacity_provider_name = testclient.create_capacity_provider(auto_scaling_group_name=auto_scaling_group_name)
        cluster_name = testclient.create_cluster(capacity_providers=[capacity_provider_name], cluster_name=cluster_name)
        return cluster_name

if __name__ == "__main__":
    testclient = ECSClient(region_name="us-east-2")
    time.sleep(15)
    cluster_name = testclient.create_auto_scaling_cluster()
    container_instances = testclient.spin_til_containers_up(cluster_name)
    testclient.set_and_register_task(
        ["echo start"], ["/bin/bash", "-c"], family="multimessage"
    )
    print(cluster_name)
    time.sleep(15)
    print(testclient.exec_commands_on_containers(container_instances, ['echo hello']))
    testclient.run_task(use_launch_type=False)
    testclient.spin_til_running(time_delay=2)

    testclient.get_clusters_usage()
    testclient.terminate_containers_in_cluster(cluster_name)
