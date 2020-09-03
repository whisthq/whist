from pprint import pprint
import time
import random
import string
import paramiko
import io

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
        if starter_auto_scaling_client is None:
            self.auto_scaling_client = self.make_client("autoscaling")
        else:
            self.auto_scaling_client = starter_auto_scaling_client
        self.set_cluster(cluster_name=base_cluster)

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

    def ssh_container(self, container, ssh_command):
        instance_info = self.ec2_client.describe_instances(InstanceIds=[container])['Reservations'][0]['Instances'][0]
        pprint(instance_info)
        instance_ip = instance_info['PublicIpAddress']

        instance_key_name = instance_info['KeyName']
        instance_key_material = self.s3_client.get_object(Bucket='fractal-container-keys', Key=instance_key_name)['Body'].read().decode("utf-8")
        private_key_file = io.StringIO()
        private_key_file.write(instance_key_material)
        private_key_file.seek(0)
        private_key = paramiko.RSAKey.from_private_key(private_key_file)

        ssh_client = paramiko.SSHClient()
        ssh_client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

        # Connect/ssh to an instance
        try:
            ssh_client.connect(hostname=instance_ip, username="ubuntu", pkey=private_key)
            print(f"SSH connected to EC2 instance {instance_id} at public IP {instance_ip}")

            # Execute a command(cmd) after connecting/ssh to an instance
            stdin, stdout, stderr = ssh_client.exec_command(ssh_command)
            err = stderr.readlines()
            if not err:
                print(f"SSH success! Command output: {''.join(stdout.readlines())}")
            else:
                print(f"SSH command error: {''.join(err)}")

            # close the client connection once the job is done
            ssh_client.close()
        except Exception as e:
            print(f"SSH encountered error: {e}")
        finally:
            private_key_file.close()

    def ssh_containers_in_cluster(self, cluster, ssh_command):
        containers = self.get_containers_in_cluster(cluster)
        for container in containers:
            self.ssh_container(container, ssh_command)

    def ssh_all_containers(self, ssh_command):
        clusters = self.get_all_clusters()
        for cluster in clusters:
            self.ssh_containers_in_cluster(cluster, ssh_command)
                
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
        family="echostart",
        containername="basictest",
        imagename="httpd:2.4",
        memory="512",
        cpu="256",
    ):
        """
        Generates a task corresponding to a given docker image, command, and entry point
        while setting instance attrs accordingly (logging and task attrs).
        Args:
            command (List[str]): Command to run on container
            entrypoint (List[str]): Entrypoint for container
            family (Optional[str]): what task family you want this task revising
            containername (Optional[str]):  what you want the container the task is on to be called
            imagename (Optional[str]): the URI for the docker image
            memory (Optional[str]): how much memory (in MB) the task needs
            cpu (Optional[str]): how much CPU (in vCPU) the task needs
        """
        fmtstr = family + str(self.offset + 1)
        try:
            self.log_client.create_log_group(logGroupName="/ecs/{}".format(fmtstr))
        except botocore.exceptions.ClientError as e:
            self.warnings.append(str(e))
        #TODO:  update basedict to mimic successful task launch
        basedict = {
            "executionRoleArn": "arn:aws:iam::{}:role/ecsTaskExecutionRole".format(self.account_id),
            "containerDefinitions": [
                {
                    "logConfiguration": {
                        "logDriver": "awslogs",
                        "options": {
                            "awslogs-group": "/ecs/{}".format(fmtstr),
                            "awslogs-region": self.region_name,
                            "awslogs-stream-prefix": "ecs",
                        },
                    },
                    "entryPoint": entrypoint,
                    "portMappings": [{"hostPort": 8080, "protocol": "tcp", "containerPort": 8080}],
                    "command": command,
                    "cpu": 0,
                    "environment": [{"name": "TEST", "value": "end"}],
                    "image": imagename,
                    "name": containername,
                }
            ],
            "placementConstraints": [],
            "memory": memory,
            "family": family,
            "networkMode": "awsvpc",
            "cpu": cpu,
        }
        if basedict["containerDefinitions"][0]["command"] is None:
            basedict["containerDefinitions"][0].pop("command")
        if basedict["containerDefinitions"][0]["entryPoint"] is None:
            basedict["containerDefinitions"][0].pop("entryPoint")
        response = self.ecs_client.register_task_definition(**basedict)
        arn = response["taskDefinition"]["taskDefinitionArn"]
        self.task_definition_arn = arn
        self.container_name = containername

    def run_task(self, **kwargs):
        """
        sets this client's task running.
        TODO: explicitly add overrides as params here for cpu, command, and environment vars
        Args:
            **kwargs: Any add'l params you want the task to have
        """
        taskdict = self.ecs_client.run_task(
            taskDefinition=self.task_definition_arn,
            launchType=self.launch_type,
            cluster=self.cluster,
            **kwargs
        )
        task = taskdict["tasks"][0]
        container = task["containers"][0]
        running_task_arn = container["taskArn"]
        self.tasks.append(running_task_arn)
        self.tasks_done.append(False)
        self.offset += 1

    def stop_task(self, reason="user stopped", offset=0):
        self.ecs_client.stop_task(cluster=self.cluster, task=self.tasks[offset], reason=reason)
        self.tasks_done[offset]=True

    def create_launch_configuration(self, instance_type, ami, launch_config_name=None, key_name=None):
        if not key_name:
            key_name = self.generate_name('key')
            key_material = self.ec2_client.create_key_pair(KeyName=key_name)['KeyMaterial']
            self.s3_client.put_object(
                Key=key_name,
                Body=key_material,
                Bucket='fractal-container-keys'
            )
        launch_config_name = launch_config_name or self.generate_name('launch_configuration')
        response = self.auto_scaling_client.create_launch_configuration(
            LaunchConfigurationName=launch_config_name,
            ImageId=ami,
            InstanceType=instance_type,
            KeyName=key_name,
        )
        return launch_config_name

    def create_auto_scaling_group(self, launch_config_name, auto_scaling_group_name=None, min_size=1, max_size=10, availability_zones=None):
        availability_zones = availability_zones or [self.region_name + 'a']
        if isinstance(availability_zones, str):
            availability_zones = [availability_zones]
        if not isinstance(availability_zones, list):
            raise Exception("availability_zones should be a list of strs")
        auto_scaling_group_name = auto_scaling_group_name or self.generate_name('auto_scaling_group')
        response = self.auto_scaling_client.create_auto_scaling_group(
            AutoScalingGroupName=auto_scaling_group_name,
            LaunchConfigurationName=launch_config_name,
            MaxSize=max_size,
            MinSize=min_size,
            AvailabilityZones=availability_zones,
        )
        return auto_scaling_group_name

    def create_capacity_provider(self, auto_scaling_group_name, capacity_provider_name=None):
        auto_scaling_group_info = self.auto_scaling_client.describe_auto_scaling_groups(AutoScalingGroupNames=[auto_scaling_group_name])
        auto_scaling_group_arn = auto_scaling_group_info['AutoScalingGroups'][0]['AutoScalingGroupARN']
        capacity_provider_name = capacity_provider_name or self.generate_name('capacity_provider')
        response = self.ecs_client.create_capacity_provider(
            name=capacity_provider_name,
            autoScalingGroupProvider={
                'autoScalingGroupArn': auto_scaling_group_arn,
            }
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
            self.task_ips[offset]=public_ip
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


if __name__ == "__main__":
    testclient = ECSClient(region_name="us-east-2")
    # testclient.set_and_register_task(
    #     ["echo start"], ["/bin/bash", "-c"], family="multimessage",
    # )
    # networkConfiguration = {
    #     "awsvpcConfiguration": {
    #         "subnets": ["subnet-0dc1b0c43c4d47945",],
    #         "securityGroups": ["sg-036ebf091f469a23e",],
    #     }
    # }
    # testclient.run_task(networkConfiguration=networkConfiguration)
    # testclient.spin_til_running(time_delay=2)
    # testclient.get_clusters_usage()
    # testclient.ssh_all_containers(ssh_command='echo hello')


    launch_config_name = testclient.create_launch_configuration(instance_type='t2.micro', ami='ami-07e651ecd67a4f6d2', launch_config_name=None)
    auto_scaling_group_name = testclient.create_auto_scaling_group(launch_config_name=launch_config_name)
    capacity_provider_name = testclient.create_capacity_provider(auto_scaling_group_name=auto_scaling_group_name)
    cluster_name = testclient.create_cluster(capacity_providers=[capacity_provider_name])
    testclient.set_and_register_task(
        ["echo start"], ["/bin/bash", "-c"], family="multimessage",
    )
    # networkConfiguration = {
    #     "awsvpcConfiguration": {
    #         "subnets": ["subnet-0dc1b0c43c4d47945",],
    #         "securityGroups": ["sg-036ebf091f469a23e",],
    #     }
    # }
    #testclient.run_task(networkConfiguration=networkConfiguration)
    testclient.run_task()
    testclient.spin_til_running(time_delay=2)
    testclient.get_clusters_usage()

    container_instances = []
    # while not container_instances:
    #     container_instances = testclient.get_containers_in_cluster()
        # spin
    
    
    # testclient.ssh_containers_in_cluster('cluster_jhchrdngkg', ssh_command='echo hello')
    # testclient.terminate_containers_in_cluster('cluster_jhchrdngkg')

    testclient.ssh_container('i-0f5f840ab7437c672', ssh_command='echo hello')
    # print(cluster_name)
    # print(testclient.task_ips)