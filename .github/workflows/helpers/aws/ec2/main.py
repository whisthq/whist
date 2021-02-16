import boto3
import os
import pandas as pd
import slack


def get_instances(region):
    """Reads running EC2 instances and formats a slack message to send

    Returns:
        A dictionary containing two keys, 'instances' and 'message,' where instances
        contains all the instances available and 'message' contains the slack message
        to be sent to the Fractal slack alerts channel
    """
    message = ""
    ec2_instances = []
    client = boto3.client("ec2", region_name=region)
    response = client.describe_instances()
    reservations = response["Reservations"]
    for r in reservations:
        instances = r["Instances"]
        for instance in instances:
            row = {}
            instance_id = instance["InstanceId"]
            state = instance["State"]["Name"]
            row["Instance ID"] = instance_id
            row["Instance State"] = state
            icon = ":red_circle" if state == "stopped" else ":white_check_mark:"
            message += "• " + instance["InstanceId"] + " - " + state + icon + " \n"
            if "Tags" in instance:
                for tag in instance["Tags"]:
                    if tag["Key"] == "Name":
                        row["tag-name"] = tag["Value"]

            ec2_instances.append(row)

    ret = dict()
    ret["instances"] = ec2_instances
    ret["message"] = message
    return ret


if __name__ == "__main__":
    # slack message formatter
    blocks = [
        {
            "type": "section",
            "text": {
                "type": "mrkdwn",
                "text": "*EC2 Instance Status for: {}*".format(region),
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
    region = os.environ.get("AWS_REGION")
    obj = get_instances(region)
    df_ec2 = pd.DataFrame(obj["instances"])
    client = slack.WebClient(token=os.environ.get("SLACK_EC2_BOT_TOKEN"))
    blocks[1]["text"]["text"] = (
        obj["message"] if len(obj["message"]) > 0 else "No Instances available"
    )
    client.chat_postMessage(channel="U01J21MUCMS", blocks=blocks)
