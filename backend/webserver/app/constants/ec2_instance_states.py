from enum import Enum


class EC2InstanceState(str, Enum):
    # We invent this one to represent this possible state instead of re-raising
    # the exception returned by boto3.
    DOES_NOT_EXIST = "does_not_exist"

    # This is the list of possible EC2-provided statuses for an instance.
    # List from https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/ec2-instance-lifecycle.html
    PENDING = "pending"
    RUNNING = "running"
    STOPPING = "stopping"
    STOPPED = "stopped"
    SHUTTING_DOWN = "shutting-down"
    TERMINATED = "terminated"
