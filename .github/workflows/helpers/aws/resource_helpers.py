#!/usr/bin/env python3

from datetime import date, datetime, timedelta, timezone
import os
import sys
import subprocess
import json
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
        client = boto3.client("ecs", region_name=region)
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


def get_all_aws_asgs(region):
    """
    Reads all Auto Scaling Groups in AWS for a specified region

    Args:
        region (str): target region

    Returns:
        list: list of all Auto Scaling Groups in region
    """
    client = boto3.client("autoscaling", region_name=region)
    response = client.describe_auto_scaling_groups()
    all_asgs = [asg["AutoScalingGroupARN"] for asg in response["AutoScalingGroups"]]
    return all_asgs


def get_all_aws_clusters(region):
    """
    Reads all clusters in AWS for a specified region

    Args:
        region (str): target region

    Returns:
        list: list of all clusters in region
    """
    client = boto3.client("ecs", region_name=region)
    response = client.list_clusters()

    all_cluster_arns = response["clusterArns"]
    return all_cluster_arns


def cluster_to_asgs(cluster_arn, region):
    """
    Takes a Cluster ARN and maps it to its Auto Scaling Group(s) via its
    associated Capacity Provider(s)

    Args:
        cluster_arn (str): full cluster ARN
        region (str): region where cluster is running

    Returns:
        list: list of ASGs associated with a provided cluster
    """
    client = boto3.client("ecs", region_name=region)
    response = client.describe_clusters(clusters=[cluster_arn])

    capacity_providers = response["clusters"][0]["capacityProviders"]
    if len(capacity_providers) > 0:
        response = client.describe_capacity_providers(capacityProviders=capacity_providers)
        asgs = [
            asg["autoScalingGroupProvider"]["autoScalingGroupArn"]
            for asg in response["capacityProviders"]
            if "autoScalingGroupProvider" in asg
        ]
        return asgs
    else:
        return []


def get_db_clusters(url, secret, region):
    """
    Queries specified db for all clusters in a specified region

    Args:
        url (str): database url to query
        secret (str): x-hasura-admin-secret
        region (str): target region

    Returns:
        list: list of clusters
    """
    clusters, _ = subprocess.Popen(
        [
            "curl",
            "-L",
            "-X",
            "POST",
            url,
            "-H",
            "Content-Type: application/json",
            "-H",
            "x-hasura-admin-secret: %s" % (secret),
            "--data-raw",
            (
                '{"query":"query get_clusters($_eq: String = \\"%s\\")'
                ' { hardware_cluster_info(where: {location: {_eq: $_eq}}) { cluster }}"}'
            )
            % (region),
        ],
        stdout=subprocess.PIPE,
    ).communicate()
    clusters = json.loads(clusters)["data"]["hardware_cluster_info"]
    return [list(cluster.values())[0] for cluster in clusters]


def get_hanging_asgs(region):
    """
    Compares list of ASGs in AWS to list of ASGs associated with Clusters in AWS

    Args:
        region (str): target region

    Returns:
        list: list of ASG names
    """
    cluster_asgs = [
        asg
        for cluster_arn in get_all_aws_clusters(region)
        for asg in cluster_to_asgs(cluster_arn, region)
        if cluster_arn is not None
    ]
    all_asgs = get_all_aws_asgs(region)

    # return names (not ARNs) of ASGs not assocated with a cluster
    # (could be more than one ASG per cluster)
    return [asg.split("/")[-1] for asg in list(set(all_asgs) - set(cluster_asgs))]


def get_hanging_clusters(urls, secrets, region):
    """
    Compares list of clusters in AWS to list of clusters in all DBs

    Args:
        urls (list[str]): database urls to query
        secrets (list[str]): x-hasura-admin-secrets
        region (str): target region

    Returns:
        list: list of cluster names
    """
    # parses names of clusters from ARNs since only the names are stored in the DB
    aws_clusters = {cluster.split("/")[-1] for cluster in get_all_aws_clusters(region)}
    if "default" in aws_clusters:
        aws_clusters.remove("default")

    # loop through each of dev, staging, and prod; add clusters to db_clusters
    db_clusters = set()
    for url, secret in zip(urls, secrets):
        db_clusters |= set(get_db_clusters(url, secret, region))

    # return list of cluster names in aws but not in db (and vice versa),
    # ignoring the 'default' cluster
    return [list(aws_clusters - db_clusters), list(db_clusters - aws_clusters)]


def get_hanging_tasks(urls, secrets, region):
    """
    Compares list of tasks associated with clusters in AWS to list of tasks in all DBs

    Args:
        urls (list[str]): database urls to query
        secrets (list[str]): x-hasura-admin-secrets
        region (str): target region

    Returns:
        list: list of cluster names
    """
    # query dbs for tasks and clusters
    db_clusters = []
    db_tasks = set()
    for url, secret in zip(urls, secrets):
        db_clusters += get_db_clusters(url, secret, region)
        db_tasks |= set(get_db_tasks(url, secret, region))

    aws_tasks = set()
    aws_tasks_and_times = {}
    for cluster in db_clusters:
        # verify cluster exists in aws (prevents later exception)
        client = boto3.client("ecs", region_name=region)
        response = client.describe_clusters(clusters=[cluster])
        if len(response["clusters"]) > 0:
            # get all tasks in a cluster (if it exists)
            response = client.list_tasks(cluster=cluster)
            tasks = response["taskArns"]
            aws_tasks |= set(tasks)
            # if there are tasks in a cluster, get created time
            if len(tasks) > 0:
                response = client.describe_tasks(cluster=cluster, tasks=tasks)
                tasks_and_times = (
                    (task["taskArn"], (cluster, task["createdAt"])) for task in response["tasks"]
                )
                aws_tasks_and_times.update(dict(tasks_and_times))

    # determine which tasks are in aws but not in the db and return them (and their creation time)
    hanging_tasks = list(aws_tasks - db_tasks)
    return [
        (
            task,
            delete_if_older_than_one_day(
                region, task, aws_tasks_and_times[task][0], aws_tasks_and_times[task][1]
            ),
        )
        for task in hanging_tasks
    ]


def flag_instances(region):
    """
    Flags all the instances in a given region

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


def hanging_resource(component, region, urls, secrets):
    """
    Reports any hanging resources of the specified component in a given region

    Args:
        component (str): one of [ASGs, Clusters, Tasks, Instances]
        region (str): current region
        urls (list[str]): database urls to query
        secrets (list[str]): x-hasura-admin-secrets

    Returns:
        string: formatted bulleted list for Slack notification
    """
    if component == "ASGs":
        asgs = get_hanging_asgs(region)
        if len(asgs) > 0:
            return "\n     - `" + "`\n     - `".join([str(x) for x in asgs]) + "`"
        return ""
    elif component == "Clusters":
        output = []
        aws_clusters, db_clusters = get_hanging_clusters(urls, secrets, region)
        for cluster in aws_clusters:
            output.append((str(cluster), get_num_instances(cluster, region)))
        msg = ""
        if len(aws_clusters) > 0:
            msg += "\n     - " + "\n     - ".join(
                [
                    "`" + c + "`" + " in AWS but not in any DB (" + str(n) + " instances)"
                    for c, n in output
                ]
            )
        if len(db_clusters) > 0:
            msg += "\n     - " + "\n     - ".join(
                ["`" + c + "`" + " in a DB but not AWS" for c in db_clusters]
            )
        return msg
    elif component == "Tasks":
        tasks = get_hanging_tasks(urls, secrets, region)
        if len(tasks) > 0:
            return "\n     - " + "\n     - ".join(["`" + str(t) + "`" + d for t, d in tasks])
        return ""
    elif component == "Instances":
        msg = "\n"
        instances = flag_instances(region)
        if len(instances) > 0:
            msg += instances
            return msg
        return ""
