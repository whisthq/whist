from pprint import pprint
import time

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

    def set_cluster(self, cluster_name=None):
        """
        sets the task's compute cluster to be the first available/default compute cluster.
        """
        self.cluster = (
            check_str_param(cluster_name, 'cluster_name')
            if cluster_name
            else self.ecs_client.list_clusters()["clusterArns"][0]
        )

    def set_and_register_task(
        self,
        command,
        entrypoint,
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

    testclient = ECSClient()
    testclient.set_and_register_task(
        ["echo start"], ["/bin/bash", "-c"], family="multimessage",
    )
    networkConfiguration = {
        "awsvpcConfiguration": {
            "subnets": ["subnet-0dc1b0c43c4d47945",],
            "securityGroups": ["sg-036ebf091f469a23e",],
        }
    }
    testclient.run_task(networkConfiguration=networkConfiguration)
    testclient.spin_til_running(time_delay=2)
    print(testclient.task_ips)
