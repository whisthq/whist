#!/usr/bin/env python3

from datetime import date, datetime, timedelta, timezone
import os
import sys
import boto3

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


def has_elapsed_hours(timestamp, hours):
    """
    Determines if the time elapsed since timestamp exceeds a given number of hours

    Args:
        timestamp (datetime): datetime object representing the time a resource was created
        hours (int): number of hours to check if elapsed

    Returns:
        bool: a boolean representing whether provided num of hours have elapsed since timestamp
    """
    return abs(datetime.now(timezone.utc) - timestamp) > timedelta(hours=hours)


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


def read_tags(tags, resource):
    """
    Reads tags for a given EC2 resource and returns tags

    Args:
        tags (arr): array of key value pairs
        resource (str): current AWS EC2 resource

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


def delete_if_older_than_one_day(region, task, cluster, time):
    """
    Determines if provided task is older than one day and stops it

    Args:
        region (str): region of where task/cluster is running
        task (str): task ARN of task to check
        cluster (str): cluster that the specified task is associated with
        time (datetime): datetime object representing the timestamp an task was started

    Returns:
        str: indicator of whether stop was triggered or not
    """
    if has_elapsed_hours(time, 24):
        client = boto3.client("ec2", region_name=region)
        client.stop_task(
            cluster=cluster,
            task=task,
            reason="Automatically stopped by resource cleanup script (older than 1 day).",
        )
        return " (stop triggered)"
    return ""


def get_all_aws_instances(region):
    """
    Gets all instances (and their info) from a given region

    Args:
        region (str): current region

    Returns:
        arr: array containing all instances (and info) from the given region
    """
    client = boto3.client("ec2", region_name=region)
    response = client.describe_instances()

    return response["Reservations"]


def num_aws_instances(region):
    """
    Gets number of all instances in a given region

    Args:
        region (str): current region

    Returns:
        tuple(int, int): (# running instances, total # instances)
    """
    reservations = get_all_aws_instances(region)
    running_instances = sum(
        [
            1 if instance["State"]["Code"] == 16 else 0
            for res in reservations
            for instance in res["Instances"]
        ]
    )
    total_instances = sum([len(res["Instances"]) for res in reservations])

    return (running_instances, total_instances)


def flag_instances(region):
    """
    Flags all the EC2 instances in a given region

    Args:
        region (str): current region

    Returns:
        map: map of all branches with associated instances
    """
    reservations = get_all_aws_instances(region)
    msg = ""

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
                msg += f"     - `{name}` - id: `{instance_id}` - *UPTIME:* {days} days\n"
            elif test:
                if has_elapsed_hours(launch_time, 1) and state not in shutting_down_states:
                    msg += f"     - `{name}` - id: `{instance_id}` - *TEST INSTANCE OVERDUE*\n"
            elif len(name) == 0 and state not in shutting_down_states:
                msg += f"     - id: `{instance_id}` - *UNTAGGED/UNNAMED*\n"

    return msg
