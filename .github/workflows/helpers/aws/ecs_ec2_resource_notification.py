from datetime import datetime
from datetime import date
from datetime import timezone
from datetime import timedelta

import boto3

regions = [
    "us-east-1",
    "us-east-2",
    "us-west-1",
    "us-west-2",
    "ca-central-1",
    "eu-west-1",
    "eu-central-1",
]


def read_tags(tags, resource):
    """
    Reads tags for a given resource, either EC2 or ECS and returns tags

    Args:
        tags (arr): array of key value pairs
        resource (str): current aws resource, either EC2 or ECS

    Returns:
        tuple: the branch, commit, name of the resource, and whether it was created on test
    """
    name = ""
    test = "False"
    key = "Key" if resource == "EC2" else "key"
    value = "Value" if resource == "EC2" else "value"
    for tag in tags:
        if tag[key] == "created_on_test":
            test = tag[value]
        if tag[key] == "Name":
            name = tag[value]

    return name, test == "True"


def compare_days(timestamp):
    """
    Compares a given timestamp with the current date and returns whether
    the timestamp is more than 2 weeks old
    Args:
        timestamp (datetime): datetime object representing the timestamp an EC2 instance was created
    Returns:
        str: a String representing a status code, either overdue or ignore
    """
    today = date.today()
    launch_time = timestamp.date()

    different = today - launch_time

    return (True, different.days) if different.days >= 14 else (False, different.days)


def compare_hours(timestamp):
    return abs(datetime.now(timezone.utc) - timestamp) > timedelta(hours=1)


def get_all_instances(region):
    """
    Gets all instances from a given region

    Args:
        region (str): current region

    Returns:
        arr: array containing all instances from the given region
    """
    client = boto3.client("ec2", region_name=region)
    response = client.describe_instances()

    return response["Reservations"]


def flag_instances(region):
    """
    Flags all the instances in a given region

    Args:
        region (str): current region

    Returns:
        map: map of all branches with associated instances
    """
    reservations = get_all_instances(region)
    message = ""

    shutting_down_states = ["shutting-down", "terminated", "stopping", "stopped"]

    for res in reservations:
        instances = res["Instances"]
        for instance in instances:
            launch_time = instance["LaunchTime"]

            name = ""
            test = False
            instance_id = instance["InstanceId"]
            state = instance["State"]["Name"]

            if "Tags" in instance:
                name, test = read_tags(instance["Tags"], "EC2")

            overdue, days = compare_days(launch_time)
            if overdue and state not in shutting_down_states:
                message += (
                    f"     - `{name}` - id: `{instance_id}` - *UPTIME:* {days} days\\n"
                )
            elif test:
                if compare_hours(launch_time) and state not in shutting_down_states:
                    message += f"     - `{name}` - id: `{instance_id}` - *TEST INSTANCE OVERDUE*\\n"
            elif len(name) == 0 and state not in shutting_down_states:
                message += f"     - id: `{instance_id}` - *UNTAGGED/UNNAMED*\\n"

    return message


if __name__ == "__main__":
    msg = ""
    for reg in regions:
        msg += "Instances from *{}*\\n".format(reg)
        reg_instances = flag_instances(reg)
        msg += (
            flag_instances(reg)
            if len(reg_instances) > 0
            else "     - No hanging instances\\n"
        )
        msg += "\\n"

    if len(msg) > 0:
        print(msg)
    else:
        print("No hanging instances!")
