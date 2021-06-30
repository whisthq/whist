import os
import random
import re
import string
import time
from collections import defaultdict
from datetime import date
from pprint import saferepr
from typing import List, Dict


import boto3
import botocore.exceptions
from flask import current_app

from app.helpers.utils.general.logs import fractal_logger
from app.constants.ec2_userdata_template import userdata_template

from . import ecs_deletion
from . import autoscaling


class FractalECSClusterNotFoundException(Exception):
    """
    Exception to be raised when a requested cluster is not found on ECS.
    """

    def __init__(self, cluster_name):
        super().__init__()
        self.cluster_name = cluster_name

    def __str__(self):
        return "FractalECSClusterNotFoundException: Cluster {0} was not found!".format(
            self.cluster_name
        )


class ContainerBrokenException(Exception):
    pass


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
        starter_s3_client (boto3.client): starter s3 client, used for mocking
        starter_iam_client (boto3.client): starter iam client, used for mocking
        starter_auto_scaling_client (boto3.client): starter auto scaling client, used for mocking
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
        starter_iam_client=None,
        starter_auto_scaling_client=None,
        grab_logs=True,
        mock=False,
    ):
        self.key_id = check_str_param(key_id, "key_id")
        self.region_name = check_str_param(region_name, "region_name")
        self.access_key = check_str_param(access_key, "access_key")
        self.launch_type = check_str_param(launch_type, "launch_type")
        self.account_id = "123412341234"
        self.subnet = None
        self.vpc = None
        self.security_groups = None
        self.grab_logs = check_bool_param(grab_logs, "grab_logs")
        self.task_definition_arn = None
        self.tasks = []
        self.task_ips = {}
        self.task_ports = {}
        self.tasks_done = []
        self.offset = -1
        self.container_name = None
        self.ecs_logs = dict()
        self.logs_messages = dict()
        self.warnings = []
        self.cluster = None
        self.mock = mock

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
        if starter_iam_client is None:
            self.iam_client = self.make_client("iam")
        else:
            self.iam_client = starter_iam_client
        if starter_auto_scaling_client is None:
            self.auto_scaling_client = self.make_client("autoscaling")
        else:
            self.auto_scaling_client = starter_auto_scaling_client
        try:
            self.set_cluster(cluster_name=base_cluster)
        except Exception as e:
            print(e)
            self.cluster = None

        if not self.mock:
            self.role_name = "autoscaling_role"
            self.instance_profile = "auto_scaling_instance_profile"

    def make_client(self, client_type, **kwargs):
        """
        Constructs an ECS client object with the given params
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
            **kwargs,
        )
        return clients

    @staticmethod
    def _get_git_info():
        branch = current_app.config["APP_GIT_BRANCH"]
        commit = current_app.config["APP_GIT_COMMIT"]

        return branch, commit[0:7]

    @staticmethod
    def generate_name(starter_name="", test_prefix=False):
        """
        Helper function for generating a name with a random UID
        Args:
            starter_name (Optional[str]): starter string for the name
            test_prefix (Optional[bool]): whether the resource should have a "test-" prefix attached
        Returns:
            str: the generated name
        """
        branch, commit = ECSClient._get_git_info()
        # Sanitize branch name to prevent complaints from boto. Note from the
        # python3 docs that if the hyphen is at the beginning or end of the
        # contents of `[]` then it should not be escaped.
        branch = re.sub("[^0-9a-zA-Z-]+", "-", branch, count=0, flags=re.ASCII)

        # Generate our own UID instead of using UUIDs since they make the
        # resource names too long (and therefore tests fail). We would need
        # log_36(16^32) = approx 25 digits of lowercase alphanumerics to get
        # the same entropy as 32 hexadecimal digits (i.e. the output of
        # uuid.uuid4()) but 10 digits gives us more than enough entropy while
        # saving us a lot of characters.
        uid = "".join(random.choice(string.ascii_lowercase + string.digits) for _ in range(10))

        # Note that capacity providers and clusters do not allow special
        # characters like angle brackets, which we were using before to
        # separate the branch name and commit hash. To be safe, we don't use
        # those characters in any of our resource names.
        name = f"{starter_name}-{branch}-{commit}-uid-{uid}"

        test_prefix = test_prefix or current_app.testing
        if test_prefix:
            name = f"test-{name}"

        return name

    def create_cluster(self, capacity_providers, cluster_name=None):
        """Create a new cluster with the specified capacity providers.

        Args:
            capacity_providers (List[str]): capacity providers to use for cluster
            cluster_name (Optional[str]): name of cluster, will be automatically generated if not
                provided
        """
        branch, commit = ECSClient._get_git_info()

        if isinstance(capacity_providers, str):
            capacity_providers = [capacity_providers]
        if not isinstance(capacity_providers, list):
            raise Exception("capacity_providers must be a list of strs")
        cluster_name = cluster_name or ECSClient.generate_name("cluster")

        self.ecs_client.create_cluster(
            clusterName=cluster_name,
            tags=[
                {"key": "git_branch", "value": branch},
                {"key": "git_commit", "value": commit},
                {"key": "created_on_test", "value": "True" if current_app.testing else "False"},
                {"key": "created_at", "value": date.today().strftime("%Y-%d-%m")},
            ],
            capacityProviders=capacity_providers,
            defaultCapacityProviderStrategy=[
                {
                    "capacityProvider": capacity_provider,
                    "weight": 1,
                    "base": 0,
                }
                for capacity_provider in capacity_providers
            ],
        )
        self.set_cluster(cluster_name)
        return cluster_name

    def set_cluster(self, cluster_name=None):
        """Set the task's compute cluster.

        Choose either specified cluster or the first available/default compute cluster if a cluster
        is not provided.

        Args:
            cluster_name (Optional[str]): name of cluster to set task's compute cluster to
        """

        self.cluster = (
            check_str_param(cluster_name, "cluster_name")
            if cluster_name
            else self.ecs_client.list_clusters()["clusterArns"][0]
        )

    def get_all_clusters(self):
        """
        Returns:
            List[str]: list of all cluster ARNs owned by AWS account
        """
        clusters, next_token = [], None
        while True:
            clusters_response = (
                self.ecs_client.list_clusters(next_token)
                if next_token
                else self.ecs_client.list_clusters()
            )
            clusters.extend(clusters_response["clusterArns"])
            next_token = (
                clusters_response["nextToken"] if "nextToken" in clusters_response else None
            )
            if not next_token:
                break
        return clusters

    def describe_cluster(self, cluster: str) -> Dict:
        """
        Gets the raw JSON (as dict) description of a cluster.

        Args:
            cluster: cluster todescribe

        """
        return self.ecs_client.describe_clusters(clusters=[cluster])["clusters"][0]

    def get_auto_scaling_groups_in_cluster(self, cluster: str) -> List[str]:
        """
        Get the name of all ASGs in a cluster.
        """
        capacity_providers = self.ecs_client.describe_clusters(clusters=[cluster])["clusters"][0][
            "capacityProviders"
        ]
        capacity_providers_info = self.ecs_client.describe_capacity_providers(
            capacityProviders=capacity_providers
        )["capacityProviders"]
        auto_scaling_groups = list(
            map(
                # this graps the autoscaling group name from ARN, as the API needs the name
                lambda cp: cp["autoScalingGroupProvider"]["autoScalingGroupArn"].split("/")[-1],
                capacity_providers_info,
            )
        )
        return auto_scaling_groups

    def set_auto_scaling_group_capacity(self, asg_name: str, desired_capacity: int):
        """
        Set the desired capacity (number of instances) of an ASG

        Args:
            asg_name: name of asg to change capacity
            desired_capacity: new capacity
        """
        self.auto_scaling_client.set_desired_capacity(
            AutoScalingGroupName=asg_name,
            DesiredCapacity=desired_capacity,
            HonorCooldown=False,
        )

    def describe_auto_scaling_groups_in_cluster(self, cluster):
        """
        Args:
            cluster (str): name of cluster to set task's compute cluster to
        Returns:
            List[Dict]: each dict contains details about an auto scaling group in the cluster
        """
        capacity_providers = self.ecs_client.describe_clusters(clusters=[cluster])["clusters"][0][
            "capacityProviders"
        ]
        capacity_providers_info = self.ecs_client.describe_capacity_providers(
            capacityProviders=capacity_providers
        )["capacityProviders"]

        auto_scaling_groups = list(
            map(
                lambda cp: cp["autoScalingGroupProvider"]["autoScalingGroupArn"].split("/")[-1],
                capacity_providers_info,
            )
        )

        return self.auto_scaling_client.describe_auto_scaling_groups(
            AutoScalingGroupNames=auto_scaling_groups
        )["AutoScalingGroups"]

    def list_container_instances(self, cluster: str) -> List[str]:
        """
        Args:
            cluster: name of cluster

        Returns:
            A list of container ARNs
        """
        resp = self.ecs_client.list_container_instances(
            cluster=cluster,
        )
        key = "containerInstanceArns"
        if key not in resp:
            raise ValueError(
                f"""Unexpected AWS API response to list_container_instances.
                                Expected key {key}. Got: {resp}."""
            )
        return resp[key]

    def describe_container_instances(self, cluster: str, container_arns: List[str]) -> List[Dict]:
        """
        Args:
            cluster: name of cluster
            container_arns: list of container ARNs

        Returns:
            JSON response (as dict) description for each container
        """
        resp = self.ecs_client.describe_container_instances(
            cluster=cluster,
            containerInstances=container_arns,
        )
        key = "containerInstances"
        if key not in resp:
            raise ValueError(
                f"""Unexpected AWS API response to describe_container_instances.
                                Expected key {key}. Got: {resp}."""
            )
        return resp[key]

    def get_container_instance_ips(self, cluster, containers):
        """
        Args:
            cluster (str): name of cluster
            containers (List[str]): either the full ARNs or ec2 instance IDs of the containers
        Returns:
            List[str]: the public IP address of each container
        """
        ec2_ids = self.get_container_instance_ids(cluster, containers)
        ec2_info = self.ec2_client.describe_instances(InstanceIds=ec2_ids)
        return list(
            map(
                lambda info: info.get("PublicIpAddress", -1),
                ec2_info["Reservations"][0]["Instances"],
            )
        )

    def get_container_instance_ids(self, cluster, containers):
        """
        Args:
            cluster (str): name of cluster
            containers (List[str]): either the full ARNs or ec2 instance IDs of the containers
        Returns:
            List[str]: the ec2 instance IDs of the containers
        """
        full_containers_info = self.ecs_client.describe_container_instances(
            cluster=cluster, containerInstances=containers
        )["containerInstances"]
        return list(map(lambda info: info["ec2InstanceId"], full_containers_info))

    def get_containers_in_cluster(self, cluster):
        """
        Args:
            cluster (str): name of cluster
        Returns:
            List[str]: the ARNs of the containers in the cluster
        """
        return self.ecs_client.list_container_instances(cluster=cluster)["containerInstanceArns"]

    def get_container_for_tasks(self, task_arns=None, cluster=None):
        if task_arns is None:
            task_arns = self.tasks
        if cluster is None:
            cluster = self.cluster
        resp = self.ecs_client.describe_tasks(cluster=cluster, tasks=task_arns)

        container_arns = [task["containerInstanceArn"] for task in resp["tasks"]]
        return container_arns

    def set_containers_to_draining(self, containers, cluster=None):
        """
        sets input list of containers to draining
        Args:
            containers (list[str]): the ARNs of the containers you want to drain
            cluster (optional[str]): the cluster on which the containers are, defaults to the
                overall client's cluster.

        Returns: the json returned by the API

        """
        if len(containers) == 0:
            return
        if cluster is None:
            cluster = self.cluster

        start = 0
        end = min(start + 10, len(containers))
        while start < len(containers):
            # it was discovered that at most 10 containers can be passed, otherwise this fails
            resp = self.ecs_client.update_container_instances_state(
                cluster=cluster, containerInstances=containers[start:end], status="DRAINING"
            )
            start = end
            end = min(start + 10, len(containers))
        return resp

    def terminate_containers_in_cluster(self, cluster):
        """Terminate all of the containers in the cluster.

        They will eventually be cleaned up automatically.

        Args:
            cluster (str): name of cluster
        """

        container_arns = self.get_containers_in_cluster(cluster)
        if container_arns:
            containers = self.get_container_instance_ids(cluster, container_arns)
            self.ec2_client.terminate_instances(InstanceIds=containers)

    def get_clusters_usage(self, clusters=None):
        """Fetch usage info of clusters.

        Fetched data includes status, pending tasks, running tasks, container instances, container
        usage, min containers, and max containers.

        Args:
            clusters (Optional[List[str]]): the clusters to get the usage info for, defaults to all
                clusters associated with AWS account

        Returns:
            Dict[Dict]: A dictionary mapping each cluster name to cluster usage info, which is
                stored in a dict.
        """

        clusters, clusters_usage = clusters or self.get_all_clusters(), {}
        for cluster in clusters:
            cluster_info = self.ecs_client.describe_clusters(clusters=[cluster])["clusters"][0]
            containers = self.get_containers_in_cluster(cluster)
            auto_scaling_group_info = self.describe_auto_scaling_groups_in_cluster(cluster)[0]
            max_resources = defaultdict(int)
            for container in containers:
                instance_info = self.ecs_client.describe_container_instances(
                    cluster=cluster, containerInstances=[container]
                )["containerInstances"][0]
                for resource in instance_info["remainingResources"]:
                    if resource["name"] == "CPU" or resource["name"] == "MEMORY":
                        max_resources[resource["name"]] = max(
                            max_resources[resource["name"]], resource["integerValue"]
                        )

            clusters_usage[cluster_info["clusterName"]] = {
                "status": cluster_info["status"],
                "pendingTasksCount": cluster_info["pendingTasksCount"],
                "runningTasksCount": cluster_info["runningTasksCount"],
                "registeredContainerInstancesCount": cluster_info[
                    "registeredContainerInstancesCount"
                ],
                "maxCPURemainingPerInstance": max_resources["CPU"],
                "maxMemoryRemainingPerInstance": max_resources["MEMORY"],
                "minContainers": auto_scaling_group_info["MinSize"],
                "maxContainers": auto_scaling_group_info["MaxSize"],
            }

        fractal_logger.info(f"Cluster usage: {dict(clusters_usage)}")
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
        """Generate a task.

        Generates a task corresponding to a given docker image, command, and entry point
        while setting instance attrs accordingly (logging and task attrs).

        Args:
            command (List[str]): Command to run on container
            entrypoint (List[str]): Entrypoint for container
            basedict (Optional[Dict[str, Any]]): the base parametrization of your task.
            port_mappings (Optional[List[Dict]): any port mappings you want on the host container,
                defaults to 8080 tcp.
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
                "networkMode": "bridge",
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

    def set_task_definition_arn(self, task_arn):
        self.task_definition_arn = task_arn

    def add_task(self, task_arn):
        """Add a task by ARN to this client's task list.

        Useful for keeping an eye on already running tasks.

        Args:
            task_arn: the ID of the task to add.
        """

        self.tasks.append(task_arn)
        self.tasks_done.append(False)
        self.offset += 1

    def describe_task(self) -> dict:
        """
        Get info on the task.

        Returns:
            task_info: a dict of task information. See
            https://boto3.amazonaws.com/v1/documentation/api/latest/reference/services/ecs.html#ECS.Client.describe_task_definition
            for the response syntax.
        """
        fractal_logger.info(f"Getting task info for {self.task_definition_arn}.")
        return self.ecs_client.describe_task_definition(taskDefinition=self.task_definition_arn)

    def run_task(self, use_launch_type=True, **kwargs):
        """
        sets this client's task running.
        TODO: explicitly add overrides as params here for cpu, command, and environment vars
        Args:
            **kwargs: Any add'l params you want the task to have
        """
        task_args = {"taskDefinition": self.task_definition_arn, "cluster": self.cluster}
        if use_launch_type:
            task_args["launchType"] = self.launch_type
        taskdict = self.ecs_client.run_task(**task_args, **kwargs)
        fractal_logger.info(taskdict)
        task = taskdict["tasks"][0]
        running_task_arn = task["taskArn"]
        self.tasks.append(running_task_arn)
        self.tasks_done.append(False)
        self.offset += 1

    def stop_task(self, reason="user stopped", offset=0):
        self.ecs_client.stop_task(
            cluster=self.cluster,
            task=(self.tasks[offset]),
            reason=reason,
        )

    def create_launch_configuration(
        self,
        instance_type="g4dn.xlarge",
        ami="ami-0c82e2febb87e6d1c",
        launch_config_name=None,
        cluster_name=None,
        key_name="auto-scaling-key",
    ):
        """Add a launch configuration to our AWS account.

        Args:
            instance_type (Optional[str]): size of instances to create in auto scaling group,
                defaults to t2.small
            ami (Optional[str]): AMI to use for the instances created in auto scaling group,
                defaults to an ECS-optimized, GPU-optimized Amazon Linux 2 AMI
            launch_config_name (Optional[str]): the name to give the generated launch configuration,
                will be automatically generated if not provided
            cluster_name (Optional[str]): the cluster name that the launch configuration will be
                used for, will be automatically generated if not provided

        Returns:
            (str, str): name of cluster for launch configuration, name of launch configuration
                created
        """

        cluster_name = cluster_name or ECSClient.generate_name("cluster")

        # Initial data/scripts to be run on all container instances
        userdata = userdata_template.format(cluster_name)

        launch_config_name = launch_config_name or ECSClient.generate_name("lc")
        _ = self.auto_scaling_client.create_launch_configuration(
            LaunchConfigurationName=launch_config_name,
            ImageId=ami,
            InstanceType=instance_type,
            IamInstanceProfile=self.instance_profile,
            UserData=userdata,
            KeyName=key_name,
        )
        return cluster_name, launch_config_name

    def create_auto_scaling_group(
        self,
        launch_config_name,
        auto_scaling_group_name=None,
        min_size=0,
        max_size=10,
        availability_zones=None,
    ):
        """
        Args:
             launch_config_name (str): the launch configuration to use for the instances created by the auto scaling group
             auto_scaling_group_name (Optional[str]): the name to give the generated auto scaling group, will be automatically generated if not provided
             min_size (Optional[int]): the minimum number of containers in the auto scaling group, defaults to 1
             max_size (Optional[int]): the maximum number of containers in the auto scaling group, defaults to 10
             availability_zones (Optional[List[str]]): the availability zones for creating instances in the auto scaling group
        Returns:
             str: name of auto scaling group created
        """
        branch, commit = ECSClient._get_git_info()
        availability_zones = availability_zones or [
            self.region_name + "a" if self.region_name != "us-east-1" else "us-east-1b"
        ]
        if isinstance(availability_zones, str):
            availability_zones = [availability_zones]
        if not isinstance(availability_zones, list):
            raise Exception("availability_zones should be a list of strs")
        auto_scaling_group_name = auto_scaling_group_name or ECSClient.generate_name("asg")
        _ = self.auto_scaling_client.create_auto_scaling_group(
            AutoScalingGroupName=auto_scaling_group_name,
            LaunchConfigurationName=launch_config_name,
            MaxSize=max_size,
            MinSize=min_size,
            AvailabilityZones=availability_zones,
            NewInstancesProtectedFromScaleIn=True,
            # https://docs.aws.amazon.com/AWSCloudFormation/latest/UserGuide/aws-properties-as-tags.html
            Tags=[
                {
                    "Key": "git_branch",
                    "Value": branch,
                    "PropagateAtLaunch": True,
                },
                {
                    "Key": "git_commit",
                    "Value": commit,
                    "PropagateAtLaunch": True,
                },
                {
                    "Key": "created_on_test",
                    "Value": "True" if current_app.testing else "False",
                    "PropagateAtLaunch": True,
                },
                {"Key": "Name", "Value": ECSClient.generate_name("ec2"), "PropagateAtLaunch": True},
            ],
        )

        return auto_scaling_group_name

    def update_auto_scaling_group(self, auto_scaling_group_name, launch_config_name):
        """
        Updates a specific autoscaling group to use a new launch config
        :param auto_scaling_group_name (str): the name of the ASG to update
        :param launch_config_name (str): the name of the new launch config to use
        :return: the name of the updated ASG
        """
        _ = self.auto_scaling_client.update_auto_scaling_group(
            AutoScalingGroupName=auto_scaling_group_name,
            LaunchConfigurationName=launch_config_name,
        )

        return auto_scaling_group_name

    def create_capacity_provider(self, auto_scaling_group_name, capacity_provider_name=None):
        """
        Args:
             auto_scaling_group_name (str): the auto scaling group to create a capacity provider for
             capacity_provider_name (Optional[str]): the name to give the generated capacity provider, will be automatically generated if not provided
        Returns:
             str: name of capacity provider created
        """
        auto_scaling_group_info = self.auto_scaling_client.describe_auto_scaling_groups(
            AutoScalingGroupNames=[auto_scaling_group_name]
        )
        auto_scaling_group_arn = auto_scaling_group_info["AutoScalingGroups"][0][
            "AutoScalingGroupARN"
        ]
        capacity_provider_name = capacity_provider_name or ECSClient.generate_name("capprov")
        _ = self.ecs_client.create_capacity_provider(
            name=capacity_provider_name,
            autoScalingGroupProvider={
                "autoScalingGroupArn": auto_scaling_group_arn,
                "managedScaling": {
                    "maximumScalingStepSize": 1000,
                    "minimumScalingStepSize": 1,
                    "status": "ENABLED",
                    "targetCapacity": 100,
                },
                "managedTerminationProtection": "ENABLED",
            },
        )
        return capacity_provider_name

    def get_task_network_info(self, task_info, desired_detail):
        for attachment in task_info["attachments"]:
            if attachment["type"] == "ElasticNetworkInterface":
                for detail in attachment["details"]:
                    print(detail["name"])
                    if detail["name"] == desired_detail:
                        return detail["value"]
        raise NotImplementedError

    def get_task_binding_info(self, task_info):
        network_binding_map = {}
        for container in task_info["containers"]:
            print(container)
            for network_binding in container["networkBindings"]:
                network_binding_map[network_binding["containerPort"]] = network_binding["hostPort"]
        return network_binding_map

    def check_task_exists(self, offset=0):
        """Checks whether a task is active and actually belongs to the ecs_client cluster.

        If a task has been requested to be stopped, or if ECS failed to find the task, then
        this will tell us so. Else, we have successfully found the task.

        Args:
            offset: The offset of the task to check in `self.tasks` (default: `0`)

        Returns:
            `True` if the task exists and is not stopping, else `False`
        """
        response = self.ecs_client.describe_tasks(tasks=[self.tasks[offset]], cluster=self.cluster)

        if len(response["tasks"]) > 0 and response["tasks"][0]["desiredStatus"] == "STOPPED":
            # The container was requested to be stopped. Therefore, it is no longer valid for us.
            return False

        # If nonempty that means the task wasn't found. Else, the task is active and was found!
        return len(response["failures"]) == 0

    def get_task_ip_ports(self, offset=0):
        if self.tasks_done[offset]:
            return False
        response = self.ecs_client.describe_tasks(tasks=self.tasks, cluster=self.cluster)
        resp = response["tasks"][offset]

        # if the container is stopped, it's broken -- raise an exception
        if resp["lastStatus"] == "STOPPED":
            raise ContainerBrokenException(saferepr(response))
        elif resp["lastStatus"] == "RUNNING":
            try:
                container_instance = resp["containerInstanceArn"]
            except KeyError:
                # actually return the response as an exception so we see it in kibana
                raise ContainerBrokenException(saferepr(resp))
            container_info = self.ecs_client.describe_container_instances(
                cluster=self.cluster, containerInstances=[container_instance]
            )
            ec2_id = container_info["containerInstances"][0]["ec2InstanceId"]
            ec2_info = self.ec2_client.describe_instances(InstanceIds=[ec2_id])
            public_ip = ec2_info["Reservations"][0]["Instances"][0].get("PublicIpAddress", -1)
            self.task_ips[offset] = public_ip

            network_binding_map = self.get_task_binding_info(resp)
            if network_binding_map:
                self.task_ports[offset] = network_binding_map
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

    def spin_til_running(
        self,
        offset: int = 0,
        time_delay: int = 5,
        max_polls: int = 120,
    ) -> bool:
        """
        Spins until AWS gives us the network binding associated with a task.

        Args:
            offset: task index in self.tasks
            time_delay: seconds to sleep between AWS poll requests
            max_polls: maximum number of times to poll AWS

        Return:
            True if and only if a networking binding was returned by AWS.
        """
        for _ in range(max_polls):
            if self.get_task_ip_ports(offset=offset):
                return True
            time.sleep(time_delay)
        return False

    def spin_til_containers_up(self, cluster_name, time_delay=5):
        """
        spinpolls until container instances have been assigned to the cluster
        Args:
            cluster_name (str)
            time_delay (int): how long to wait between polls, seconds
        """
        container_instances = []
        while not container_instances:
            container_instances = self.get_containers_in_cluster(cluster_name)
            time.sleep(time_delay)

        # wait another 30 seconds just to be safe
        time.sleep(30)
        return container_instances

    def spin_til_no_containers(self, cluster_name, time_delay=5):
        """
        spinpolls until all container instances have been removed from the cluster
        Args:
            cluster_name (str)
            time_delay (int): how long to wait between polls, seconds
        """
        container_instances = []
        while container_instances:
            container_instances = self.get_containers_in_cluster(cluster_name)
            time.sleep(time_delay)

        # let AWS info propagate
        time.sleep(10)

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

    def create_auto_scaling_cluster(
        self,
        cluster_name=None,
        instance_type="t2.small",
        ami="ami-04cfcf6827bb29439",
        min_size=0,
        max_size=10,
        availability_zones=None,
    ):
        """
        Creates launch configuration, auto scaling group, capacity provider, and cluster
        Args:
            instance_type (Optional[str]): size of instances to create in auto scaling group, defaults to t2.small
            ami (Optional[str]): AMI to use for the instances created in auto scaling group, defaults to an ECS-optimized, GPU-optimized Amazon Linux 2 AMI
            min_size (Optional[int]): the minimum number of containers in the auto scaling group, defaults to 1
            max_size (Optional[int]): the maximum number of containers in the auto scaling group, defaults to 10
            region_name (Optional[str]): which AWS region you're running on
            availability_zones (Optional[List[str]]): the availability zones for creating instances in the auto scaling group
        Returns:
            (str, str, str, str): cluster name, launch configuration name, auto scaling group name, capacity provider name
        """
        cluster_name, launch_config_name = self.create_launch_configuration(
            instance_type=instance_type, ami=ami, cluster_name=cluster_name
        )
        auto_scaling_group_name = self.create_auto_scaling_group(
            launch_config_name=launch_config_name,
            min_size=min_size,
            max_size=max_size,
            availability_zones=availability_zones,
        )
        capacity_provider_name = self.create_capacity_provider(
            auto_scaling_group_name=auto_scaling_group_name
        )
        self.create_cluster(capacity_providers=[capacity_provider_name], cluster_name=cluster_name)
        return cluster_name, launch_config_name, auto_scaling_group_name, capacity_provider_name

    def update_cluster_with_new_ami(self, cluster_name, ami):
        """
        Updates a given cluster to use a new AMI
        :param cluster_name (str): which cluster to update
        :param ami (str): which AMI to use
        :return: the name of the updated cluster
        """
        self.set_cluster(cluster_name)
        try:
            launch_config_info = self.describe_auto_scaling_groups_in_cluster(self.cluster)[0]
        except (IndexError, KeyError):
            # This means that the cluster we were looking for was not found.
            raise FractalECSClusterNotFoundException(cluster_name)
        asg_name = launch_config_info["AutoScalingGroupName"]
        old_launch_config_name = launch_config_info["LaunchConfigurationName"]
        _, new_launch_config_name = self.create_launch_configuration(
            instance_type="g4dn.xlarge",
            ami=ami,
            cluster_name=cluster_name,
        )
        self.update_auto_scaling_group(asg_name, new_launch_config_name)
        containers_list = self.get_containers_in_cluster(self.cluster)
        if len(containers_list) > 0:
            self.set_containers_to_draining(containers_list)
        # Now we have to create a new undrained instance
        asg_info = self.auto_scaling_client.describe_auto_scaling_groups(
            AutoScalingGroupNames=[
                asg_name,
            ]
        )["AutoScalingGroups"][0]
        current_cap = int(asg_info["DesiredCapacity"])
        desired_cap = current_cap + 1  # start a new instance with the new ami
        if desired_cap <= asg_info["MaxSize"]:
            # if we can open a new instance, do so to handle new reqs
            # Since aws doesn't realize that we need new undrained instances
            self.auto_scaling_client.set_desired_capacity(
                AutoScalingGroupName=asg_name,
                DesiredCapacity=desired_cap,
            )
        self.auto_scaling_client.delete_launch_configuration(
            LaunchConfigurationName=old_launch_config_name
        )
        return cluster_name, ami

    @staticmethod
    def delete_cluster(cluster_name, region):
        """Tear down an ECS cluster and the entire ECS/AutoScaling stack beneath it.

        This function and all of its callees are solely responsible for sending AWS requests. They
        don't even know that Fractal maintains cluster pointers in a database.

        Args:
            cluster_name: The short name of the cluster to delete as a string.
            region: The name of the region in whicht the cluster to delete is located as a string
                (e.g. "us-east-1").

        Returns:
            None
        """

        ecs_deletion.deregister_container_instances(cluster_name, region)

        capacity_providers = ecs_deletion.delete_cluster(cluster_name, region)
        autoscaling_groups = [
            ecs_deletion.delete_capacity_provider(capacity_provider, region)
            for capacity_provider in capacity_providers
        ]

        for autoscaling_group in autoscaling_groups:
            # delete_capacity_provider() and get_launch_configuration() should return None rather
            # than raising an exception if the capacity provider to be deleted or the launch
            # configuration to retrieve respectively do not exist. They do this because requests to
            # delete AWS resources that do not exist should be considered successful. We have to
            # filter out any potential None values in this loop.
            if autoscaling_group is not None:
                launch_configuration = autoscaling.get_launch_configuration(
                    autoscaling_group, region
                )
                autoscaling.delete_autoscaling_group(autoscaling_group, region)

                if launch_configuration is not None:
                    autoscaling.delete_launch_configuration(launch_configuration, region)


if __name__ == "__main__":
    testclient = ECSClient(region_name="us-east-1")

    clusters = testclient.ecs_client.list_clusters()["clusterArns"]
    clusters_info = testclient.ecs_client.describe_clusters(clusters=clusters)["clusters"]
    for cluster_info in clusters_info:
        print(cluster_info)
        if (
            "cluster_" in cluster_info["clusterName"]
            and not cluster_info["registeredContainerInstancesCount"]
            and not cluster_info["pendingTasksCount"]
        ):
            testclient.ecs_client.delete_cluster(cluster=cluster_info["clusterName"])

    auto_scaling_instances = testclient.auto_scaling_client.describe_auto_scaling_instances()[
        "AutoScalingInstances"
    ]
    for auto_scaling_instance in auto_scaling_instances:
        print(auto_scaling_instance)
        try:
            testclient.auto_scaling_client.terminate_instance_in_auto_scaling_group(
                InstanceId=auto_scaling_instance["InstanceId"], ShouldDecrementDesiredCapacity=False
            )
        except:
            continue
    auto_scaling_groups = testclient.auto_scaling_client.describe_auto_scaling_groups()[
        "AutoScalingGroups"
    ]
    for auto_scaling_group in auto_scaling_groups:
        print(auto_scaling_group)
        try:
            testclient.auto_scaling_client.delete_auto_scaling_group(
                AutoScalingGroupName=auto_scaling_group["AutoScalingGroupName"]
            )
        except:
            continue
