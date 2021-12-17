#!/usr/bin/env python3

import sys
import os
import time
import boto3, botocore
import paramiko
from operator import itemgetter

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


def get_boto3client(region_name: str) -> botocore.client:
    # Define boto3 client with a specific region
    return boto3.client("ec2", region_name=region_name)


def get_current_AMI(boto3client: botocore.client, region_name: str) -> str:
    """
    Get the AMI of the most recent AWS EC2 Amazon Machine Image running Ubuntu Server 20.04 Focal Fossa

    Args:
        boto3client (botocore.client): The Boto3 client to use to talk to the Amazon E2 service
        region_name (str): The name of the region of interest (e.g. "us-east-1")

    Returns:
        target_ami (str): The AMI of the target image or an empty string if no image was found.
    """
    amazon_owner_id = "099720109477"
    # We will need to change the name filter once the target version of Linux Ubuntu changes
    response = boto3client.describe_images(
        Owners=[amazon_owner_id],
        Filters=[
            {
                "Name": "name",
                "Values": ["ubuntu/images/hvm-ssd/ubuntu-focal-20.04-amd64-server-*"],
            },
            {
                "Name": "architecture",
                "Values": ["x86_64"],
            },
        ],
    )
    if len(response) < 1:
        return ""
    # Sort the Images in reverse order of creation
    images_list = sorted(response["Images"], key=itemgetter("CreationDate"), reverse=True)
    # Get the AMI of the most recent Image
    target_ami = images_list[0]["ImageId"]

    return target_ami


def create_ec2_instance(
    boto3client: botocore.client,
    region_name: str,
    instance_type: str,
    instance_AMI: str,
    key_name: str,
    disk_size: int,
) -> str:
    """
    Creates an AWS EC2 instance of a specific instance type and AMI

    Args:
        instance_type (str): The type of instance to create (i.e. g4dn.2xlarge)
        instance_AMI (str): The AMI to use for the instance (i.e. ami-0b9c9d7f7f8b8f8b9)
        key_name (str): The name of the AWS key to use for connecting to the instance
        disk_size (int): The size (in GB) of the additional EBS disk volume to attach

    Returns:
        instance_id (str): The ID of the created instance
    """

    kwargs = {
        "BlockDeviceMappings": [
            {
                "DeviceName": "/dev/sda1",
                "Ebs": {"DeleteOnTermination": True, "VolumeSize": disk_size, "VolumeType": "gp3"},
            },
        ],
        "ImageId": instance_AMI,
        "InstanceType": instance_type,  # should be g4dn.2xlarge for testing the server protocol
        "MaxCount": 1,
        "MinCount": 1,
        "TagSpecifications": [
            {
                "ResourceType": "instance",
                "Tags": [
                    {"Key": "Name", "Value": "protocol-streaming-performance-testing-machine"},
                ],
            },
        ],
        "SecurityGroups": [
            "container-tester",
        ],
        "InstanceInitiatedShutdownBehavior": "terminate",
        "KeyName": key_name,  # the pre-created SSH key to associate this instance with, needs to be the same that's loaded on the client calling this function
    }

    # Create the EC2 instance
    resp = boto3client.run_instances(**kwargs)
    instance_id = resp["Instances"][0]["InstanceId"]
    print(
        f"Created EC2 instance with id: {instance_id}, type={instance_type}, ami={instance_AMI}, key_name={key_name}, disk_size={disk_size}"
    )

    return instance_id


def start_instance(boto3client: botocore.client, instance_id: str) -> bool:
    """
    Attempt to turn on an existing EC2 instance. Return a bool indicating whether the operation succeeded.

    Args:
        instance_id (str): The ID of the instance to start
    Returns:
        success (bool): indicates whether the start succeeded.
    """
    try:
        response = boto3client.start_instances(InstanceIds=[instance_id], DryRun=False)
        print(response)
    except ClientError as e:
        print("Could not start instance. Caught error: ", e)
        return False
    return True


def stop_instance(boto3client: botocore.client, instance_id: str) -> bool:
    """
    Attempt to turn off an existing EC2 instance. Return a bool indicating whether the operation succeeded.

    Args:
        instance_id (str): The ID of the instance to stop
    Returns:
        success (bool): indicates whether the start succeeded.
    """
    try:
        response = boto3client.stop_instances(InstanceIds=[instance_id], DryRun=False)
        print(response)
    except ClientError as e:
        print("Could not stop instance. Caught error: ", e)
        return False
    return True


def wait_for_instance_to_start_or_stop(
    boto3client: botocore.client, instance_id: str, stopping: bool = False
) -> None:
    """
    Hangs until an EC2 instance is reported as running or as stopped. Could be nice to make
    it timeout after some time.

    Args:
        instance_id (str): The ID of the instance to wait for
        stopping (bool): Whether or not the instance is being stopped, if not provided
            it will wait for the instance to start

    Returns:
        None
    """
    should_wait = True

    while should_wait:
        resp = boto3client.describe_instances(InstanceIds=[instance_id])
        instance_info = resp["Reservations"][0]["Instances"]
        states = [instance["State"]["Name"] for instance in instance_info]
        should_wait = False
        for _, state in enumerate(states):
            if (state != "running" and not stopping) or (state == "running" and stopping):
                should_wait = True

    print(f"Instance is{' not' if stopping else ''} running: {instance_id}")


def get_instance_ip(boto3client: botocore.client, instance_id: str) -> str:
    """
    TODO
    """
    retval = []

    # retrieve instance
    resp = boto3client.describe_instances(InstanceIds=[instance_id])
    instance = resp["Reservations"][0]

    # iterate over tags
    if instance:
        for i in instance["Instances"]:
            net = i["NetworkInterfaces"][0]
            retval.append(
                {"public": net["Association"]["PublicDnsName"], "private": net["PrivateIpAddress"]}
            )
    print(f"Instance IP is: {retval}")
    return retval
