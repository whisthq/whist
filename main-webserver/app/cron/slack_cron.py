import boto3
import os
import slack

from datetime import datetime
from datetime import date

icons = {
    "OLD COMMIT": ":red_circle:",  # commit hash for resource is old - should be deleted
    "OVERDUE": ":red_circle:",  # resource uptime has been greater than 2 weeks - should be deleted or checked
    "CREATED ON TEST": ":red_circle:",  # resource has been created on test - should be deleted
    "CURRENT COMMIT": ":large_green_circle:",  # commit matches current commit hash - ignore resource
    "IGNORE TIME": ":large_green_circle:",  # resouce uptime is < 2 weeks - ignore resource
    "NO COMMIT TAG": ":large_yellow_circle:",  # resources does not contain a commit tag - check resource to make sure if in use
}

regions = [
    "us-east-1",
    "us-east-2",
    "us-west-1",
    "us-west-2",
    "ca-central-1",
    "eu-west-1",
    "eu-central-1",
]


def get_tags(arn, region):
    """
    Gets tags for a cluster with given arn. Have to call tags separately
    because descrbe_clusters doesn't seem to return valid tags.

    Args:
        arn (str): arn of a cluster
        region (str): current region

    Returns:
        array: An array of key value pairs representing the tags of a given cluster
    """
    client = boto3.client("ecs", region_name=region)

    response = client.list_tags_for_resource(resourceArn=arn)
    return response["tags"]


def read_tags(tags, commit, branch, resource):
    """
    Reads tags for a given resource, either EC2 or ECS and returns status codes

    Args:
        tags (arr): array of key value pairs
        commit (str): current commit hash
        branch (str): current branch
        resource (str): current aws resource, either EC2 or ECS

    Returns:
        str: status code, either ignore commit, old commit, current commit, or create on test
    """
    target_branches = ["dev", "staging", "prod"]
    tag_branch = ""
    tag_commit = ""
    test = ""
    key = "Key" if resource == "EC2" else "key"
    value = "Value" if resource == "EC2" else "value"
    for tag in tags:
        if tag[key] == "created_on_test":
            test = tag[value]
        if tag[key] == "git_branch":
            tag_branch = tag[value]
        if tag[key] == "git_commit":
            tag_commit = tag[value]

    if test == "True":
        return "CREATED ON TEST"
    if branch in target_branches and tag_branch == branch:
        return "OLD COMMIT" if tag_commit != commit else "CURRENT COMMIT"

    return "NO COMMIT TAG"


def compare_timestamps(timestamp):
    """
    Compares a given timestamp with the current date and returns whether
    the timestamp is more than 2 weeks old

    Args:
        timestamp (datetime): datetime object representing the timestamp an EC2 instance was created

    Returns:
        str: a String representing a status code, either overdue or ignore
    """
    today = date.today()
    launchTime = timestamp.date()

    different = today - launchTime

    return "OVERDUE" if different.days >= 14 else "IGNORE TIME", different.days


def get_all_clusters(region):
    """
    Gets all clusters from a given region

    Args:
        region (str): string representing the given region

    Returns:
        array: an array of clusters from the given region
    """
    client = boto3.client("ecs", region_name=region)
    clusterArns = client.list_clusters()["clusterArns"]

    response = client.describe_clusters(clusters=clusterArns)
    return response["clusters"]


def flag_clusters(region, commit, branch):
    """
    Flags all the clusters in a given region

    Args:
        region (str): current region
        commit (str): current commit hash
        branch (str): current branch

    Returns:
        str: message to be sent to slack containing all flagged clusters
    """
    clusters = get_all_clusters(region)
    message = ""

    for c in clusters:
        flag = False

        clusterName = c["clusterName"]
        clusterArn = c["clusterArn"]

        tags = get_tags(clusterArn, region)

        git_status = read_tags(tags, commit, branch, "ECS")
        git_icon = icons[git_status]

        line = f"• `{clusterName}`"

        launch_status = ""
        launch_icon = ""
        days = 0
        if "created_at" in tags:
            launchTime = datetime.strptime(tags["created_at"], "%Y-%d-%m")
            launch_status, days = compare_timestamps(launchTime)
            launch_icon = icons[launch_status]

        if git_icon == ":red_circle:":
            flag = True
            line += f" - {git_status}"

        if launch_icon == ":red_circle:":
            flag = True
            line += f" - {launch_status} - uptime: {days} days "

        line += git_icon if git_icon == ":red_circle:" else launch_icon

        if len(line) > 0 and flag:
            message += f"{line} \n"

    return message


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


def flag_instances(region, commit, branch):
    """
    Flags all the instances in a given region

    Args:
        region (str): current region
        commit (str): current commit hash
        branch (str): current branch

    Returns:
        str: message to be sent to slack containing all flagged instances
    """
    reservations = get_all_instances(region)
    message = ""
    for r in reservations:
        instances = r["Instances"]
        for instance in instances:
            flag = False

            git_status = ""

            launchTime = instance["LaunchTime"]
            instance_id = instance["InstanceId"]

            launch_status, days = compare_timestamps(launchTime)
            launch_icon = icons[launch_status]

            line = f"• `{instance_id}`"

            if "Tags" in instance:
                git_status = read_tags(instance["Tags"], commit, branch, "EC2")
                git_icon = icons[git_status]

            if len(git_status) == 0:
                git_status = "NO TAGS"
                git_icon = icons["NO COMMIT TAG"]

            if git_icon == ":red_circle:":
                flag = True
                line += f"- {git_status}"

            if launch_icon == ":red_circle:":
                flag = True
                line += f" - {launch_status} - uptime: {days} days "

            if len(line) > 0 and flag:
                line += git_icon if git_icon == ":red_circle:" else launch_icon
                message += f"{line} \n"

    return message


if __name__ == "__main__":
    # region = os.environ.get("AWS_REGION")
    token = os.environ.get("SLACK_BOT_OAUTH_TOKEN")
    commit = "asdf"
    branch = "dev"
    # slack message formatter

    for resource in ["EC2", "ECS"]:
        for region in regions:
            blocks = [
                {
                    "type": "section",
                    "text": {
                        "type": "mrkdwn",
                        "text": "*{} Status for: {}*".format(resource, region),
                    },
                },
                {
                    "type": "section",
                    "text": {
                        "type": "mrkdwn",
                        "text": "• Schedule meetings \n ",
                    },
                },
            ]
            message = ""

            if resource == "EC2":
                message = flag_instances(region, commit, branch)

            elif resource == "ECS":
                message = flag_clusters(region, commit, branch)

            client = slack.WebClient(token=token)
            blocks[1]["text"]["text"] = message

            if len(message) > 0:
                client.chat_postMessage(channel="U01J21MUCMS", blocks=blocks)
