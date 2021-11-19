#!/usr/bin/env python3

import sys
import os
import time
import boto3
import paramiko

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


# Define boto3 client with a specific region
boto3client = boto3.client("ec2", region_name="us-east-1")


def create_ec2_instance(
    instance_type: str, instance_AMI: str, key_name: str, disk_size: int
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
                "Ebs": {"DeleteOnTermination": True, "VolumeSize": disk_size, "VolumeType": "gp2"},
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
        "IamInstanceProfile": {"Name": "auto_scaling_instance_profile"},
        "KeyName": key_name,  # the pre-created SSH key to associate this instance with, needs to be the same that's loaded on the client calling this function
    }

    # Create the EC2 instance
    resp = boto3client.run_instances(**kwargs)
    instance_id = resp["Instances"][0]["InstanceId"]
    print(
        f"Created EC2 instance with id: {instance_id}, type={instance_type}, ami={instance_AMI}, key_name={key_name}, disk_size={disk_size}"
    )

    return instance_id


def wait_for_instance_to_start_or_stop(instance_id: str, stopping: bool = False) -> None:
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


def get_instance_ip(instance_id: str) -> str:
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