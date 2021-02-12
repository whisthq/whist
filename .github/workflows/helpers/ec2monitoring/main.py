import boto3
import pandas as pd
import slack


blocks = [
    {"type": "section", "text": {"type": "mrkdwn", "text": "*EC2 Instance Status:*"}},
    {
        "type": "section",
        "text": {
            "type": "mrkdwn",
            "text": "• Schedule meetings \n ",
        },
    },
]

regions = [
    "us-east-1",
    "us-east-2",
    "us-west-1",
    "us-west-2",
    "eu-central-1",
    "eu-west-1",
    "ca-central-1",
]


def getInstances():
    message = ""
    ec2_instances = []
    for region in regions:
        client = boto3.client("ec2", region_name=region)
        response = client.describe_instances()
        reservations = response["Reservations"]
        for r in reservations:
            instances = r["Instances"]
            for instance in instances:
                row = {}
                row["Instance ID"] = instance["InstanceId"]
                row["region"] = region
                row["Instance State"] = instance["State"]["Name"]
                message += (
                    "• "
                    + instance["InstanceId"]
                    + " "
                    + region
                    + " "
                    + instance["State"]["Name"]
                    + " :red_circle:"
                    + " \n"
                )
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
    obj = getInstances()
    df_ec2 = pd.DataFrame(obj["instances"])
    client = slack.WebClient(
        token="xoxb-824878087478-1738745217397-gwRk3we5JOq5Gq7RHceFjBYA"
    )

    blocks[1]["text"]["text"] = obj["message"]
    client.chat_postMessage(channel="U01J21MUCMS", blocks=blocks)
