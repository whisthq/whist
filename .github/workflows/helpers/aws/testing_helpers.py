#!/usr/bin/env python3

import sys
import os
import time
import boto3
import paramiko

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


def create_ec2_instance(instance_type: str, instance_AMI: str, key_name: str) -> str:
    """
    Creates an AWS EC2 instance of a specific instance type and AMI

    Args:
        instance_type (str): The type of instance to create (i.e. g4dn.2xlarge)
        instance_AMI (str): The AMI to use for the instance (i.e. ami-0b9c9d7f7f8b8f8b9)
        key_name (str): The name of the AWS key to use for connecting to the instance

    Returns:
        instance_id (str): The ID of the created instance
    """
    kwargs = {
        "ImageId": instance_AMI,
        "InstanceType": instance_type,  # should be g4dn.2xlarge for testing the server protocol
        "MaxCount": 2,
        "MinCount": 2,
        "TagSpecifications": [
            {
                "ResourceType": "instance",
                "Tags": [
                    {"Key": "Name", "Value": "protocol-streaming-performance-testing-server"},
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
    resp = boto3.client("ec2").run_instances(**kwargs)
    instance_id = resp["Instances"][0]["InstanceId"]
    print(f"Created EC2 instance with id: {instance_id}")
    return instance_id


def wait_for_instance_to_start_or_stop(instance_ids: list(str), stopping: bool = False) -> None:
    """
    Hangs until an EC2 instance is reported as running or as stopped. Could be nice to make
    it timeout after some time.

    Args:
        instance_ids (str): The ID of the instance to wait for
        stopping (bool): Whether or not the instance is being stopped, if not provided
            it will wait for the instance to start

    Returns:
        None
    """
    should_wait = True

    while should_wait:

        print(f"id is: {instance_ids}")

        resp = boto3.client("ec2").describe_instances(InstanceIds=[instance_ids])
        instance_info = resp["Reservations"][0]["Instances"]
        states = [instance["State"]["Name"] for instance in instance_info]
        should_wait = False
        for _, state in enumerate(states):
            if (state != "running" and not stopping) or (state == "running" and stopping):
                should_wait = True

    print(f"Instance(s) is {'not' if stopping else ''} running: {instance_ids}")


def get_instance_ip(instance_id: str) -> str:
    """
    TODO
    """
    retval = []

    # retrieve instance
    resp = boto3.client("ec2").describe_instances(InstanceIds=instance_id)
    instance = resp["Reservations"]

    # iterate over tags
    if instance:
        for i in instance["Instances"]:
            net = i["NetworkInterfaces"][0]
            retval.append(
                {"public": net["Association"]["PublicDnsName"], "private": net["PrivateIpAddress"]}
            )
    print(f"Instance IP is: {retval}")
    return retval


def wait_for_ssh(instance_ip: str, ssh_key: paramiko.Ed25519Key) -> None:
    """
    Hangs until an EC2 instance has SSH initialized. Could be nice to make
    it timeout after some time.

    Args:
        instance_id (str): The ID of the instance to wait for
        ssh_key (paramiko.Ed25519Key): The SSH key used to connected to the instance

    Returns:
        None
    """
    print(f"Waiting for SSH to be available on the EC2 instance")
    ssh_client = paramiko.SSHClient()
    ssh_client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

    # Hang until the connection is established
    available = False
    while not available:
        available = True
        try:
            ssh_client.connect(hostname=instance_ip["public"], username="ubuntu", pkey=ssh_key)
        except:
            available = False
        finally:
            ssh_client.close()
        time.sleep(2)

    print(f"SSH is available on the EC2 instance: {instance_ip['public']}")


def run_ssh_command(
    ip: str,
    cmd: str,
    key: paramiko.RSAKey,
    display_res: bool,
) -> str:
    """
    TODO modify this to make it return the client command


    """
    global client_command
    ssh_client = paramiko.SSHClient()
    ssh_client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh_client.connect(hostname=ip["public"], username="ubuntu", pkey=key)
    try:
        _, stdout, _ = ssh_client.exec_command(cmd)
        if display_res:
            for line in iter(stdout.readline, ""):
                print(line, end="")
                if "linux/macos" in line:
                    client_command = line[line.find(".") + 2 :].strip()
                    print(f"Client command: {client_command}")
        else:
            for line in iter(stdout.readline, ""):
                if "linux/macos" in line:
                    client_command = line[line.find(".") + 2 :].strip()
                    print(f"Client command: {client_command}")
    except:
        pass
    finally:
        ssh_client.close()
        return client_command if client_command else ""
