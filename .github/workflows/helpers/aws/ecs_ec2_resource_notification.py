import boto3
import os
import uuid
import sys

from datetime import datetime
from datetime import date
from datetime import timezone
from datetime import timedelta


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
    for r in reservations:
        instances = r["Instances"]
        for instance in instances:
            launch_time = instance["LaunchTime"]

            name = ""
            test = False
            instance_id = instance["InstanceId"]

            if "Tags" in instance:
                name, test = read_tags(instance["Tags"], "EC2")

            overdue, days = compare_days(launch_time)
            if overdue:
                message += (
                    f"     - \\`{name}\\` - id: \\`{instance_id}\\` - UPTIME: {days} \n"
                )
            elif test:
                if compare_hours(launch_time):
                    message += f"     - \\`{name}\\` - id: \\`{instance_id}\\` - TEST INSTANCE OVERDUE \n"

            elif len(name) == 0:
                message += f"     - \\'{instance_id}\\' - UNTAGGED/UNNAMED \n"

    return message


if __name__ == "__main__":

    region = sys.argv[1]

    message = flag_instances(region)
    if len(message) > 0:
        print(message)
