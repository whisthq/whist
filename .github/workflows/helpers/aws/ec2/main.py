import boto3
import os
import pandas as pd
import slack
import sys


"""
Cluster info: 
  - we want the branch and commit hash 
    - if branch is dev, staging, or master
      - if commit hash is different from current ocmmit hash then flag 
  - 2 weeks limit: if been up for more than 2 weeks then flag 


"""


def read_tags(tags, commit, branch):
    target_branches = ["dev", "staging", "master"]
    tag_branch = ""
    tag_commit = ""

    for t in tags:
        if t["key"] == "git_branch":
            tag_branch = t["value"]
        if t["key"] == "git_commit":
            tag_commit = t["value"]

    if branch in target_branches and tag_branch == branch:
        return "OLD COMMIT" if tag_commit != commit else "CURRENT VERSION"

    return "IGNORE"


def get_all_clusters(region):
    client = boto3.client("ecs", region_name=region)
    clusterArns = client.list_clusters()["clusterArns"]

    response = client.describe_clusters(clusters=clusterArns)
    return response["clusters"]


def flag_clusters(region, commit, branch):
    clusters = get_all_clusters(region)
    flagged_clusters = []
    message = ""

    for c in clusters:
        clusterName = c["clusterName"]
        clusterArn = c["clusterArn"]

        tags = c["tags"]

        flag_status = read_tags(tags, commit, branch)
        icon = ":red_circle:" if flag_status == "OLD COMMIT" else ":white_check_mark:"
        message += "• " + clusterName + " - " + flag_status + " - " + icon + " \n"

    return message


def get_all_instances(region):
    client = boto3.client("ec2", region_name=region)
    response = client.describe_instances()

    return response["Reservations"]


def flag_instances(region, commit, branch):
    reservations = get_all_instances(region)


# def flag_instances(region, commit, branch):
#     """Reads running EC2 instances and formats a slack message to send
#     Returns:
#         A dictionary containing two keys, 'instances' and 'message,' where instances
#         contains all the instances available and 'message' contains the slack message
#         to be sent to the Fractal slack alerts channel
#     """
#     message = ""
#     ec2_instances = []
#     reservations = get_all_instances(region)
#     for r in reservations:
#         instances = r["Instances"]
#         for instance in instances:
#             row = {}
#             instance_id = instance["InstanceId"]
#             state = instance["State"]["Name"]
#             row["Instance ID"] = instance_id
#             row["Instance State"] = state
#             icon = ":red_circle:" if state == "stopped" else ":white_check_mark:"
#             message += (
#                 "• " + instance["InstanceId"] + " - " + state + " - " + icon + " \n"
#             )
#             if "Tags" in instance:
#                 for tag in instance["Tags"]:
#                     if tag["Key"] == "Name":
#                         row["tag-name"] = tag["Value"]

#             ec2_instances.append(row)

#     ret = dict()
#     ret["instances"] = ec2_instances
#     ret["message"] = message
#     return ret


if __name__ == "__main__":
    region = os.environ.get("AWS_REGION")
    token = os.environ.get("SLACK_EC2_BOT_TOKEN")
    token = "xoxb-824878087478-1738745217397-gwRk3we5JOq5Gq7RHceFjBYA"
    region = "us-east-1"
    # commit = sys.argv[1][0:7]
    # branch = sys.argv[2]

    # slack message formatter
    blocks = [
        {
            "type": "section",
            "text": {
                "type": "mrkdwn",
                "text": "*ECS Status for: {}*".format(region),
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
    # obj = flag_instances(region, commit, branch)
    # df_ec2 = pd.DataFrame(obj["instances"])
    message = flag_clusters(region, "dev", "asdf")
    client = slack.WebClient(token=token)
    blocks[1]["text"]["text"] = (
        message if len(message) > 0 else "No Instances available"
    )
    client.chat_postMessage(channel="U01J21MUCMS", blocks=blocks)