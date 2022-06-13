#!/usr/bin/env python3

import os, sys, time
import boto3
import botocore
from operator import itemgetter

from helpers.common.git_tools import get_whist_branch_name
from helpers.common.timestamps_and_exit_tools import (
    printred,
    printyellow,
)
from helpers.common.constants import (
    instances_name_tag,
    github_run_id,
    start_instance_retries,
    lock_contention_wait_time_seconds,
    lock_get_attempt_timeout_seconds,
    unique_lock_path,
    UNLOCKING_MAX_RETRIES,
)
from helpers.common.ssh_tools import (
    attempt_request_lock,
    attempt_release_lock,
)
from helpers.common.timestamps_and_exit_tools import (
    exit_with_error,
    printgrey,
)

# Add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))

# Constants
# We will need to change the owner ID/AMI once AWS' target version of Linux Ubuntu changes
AMAZON_OWNER_ID = "099720109477"
AWS_UBUNTU_2004_AMI = "ubuntu/images/hvm-ssd/ubuntu-focal-20.04-amd64-server-*"
INSTANCE_TYPE = "g4dn.xlarge"


def get_current_AMI(boto3client: botocore.client, region_name: str) -> str:
    """
    Get the AMI of the most recent AWS EC2 Amazon Machine Image running Ubuntu Server 20.04 Focal Fossa

    Args:
        boto3client (botocore.client): The Boto3 client to use to talk to the Amazon E2 service
        region_name (str): The name of the region of interest (e.g. "us-east-1")

    Returns:
        target_ami (str): The AMI of the target image or an empty string if no image was found.
    """
    response = boto3client.describe_images(
        Owners=[AMAZON_OWNER_ID],
        Filters=[
            {
                "Name": "name",
                "Values": [AWS_UBUNTU_2004_AMI],
            },
            {
                "Name": "architecture",
                "Values": ["x86_64"],
            },
        ],
    )

    if len(response) < 1:
        printred(f"Error, could not get instance AMI for region {region_name}")
        return ""

    # Sort the Images in reverse order of creation
    images_list = sorted(response["Images"], key=itemgetter("CreationDate"), reverse=True)

    # Get the AMI of the most recent Image
    target_ami = images_list[0]["ImageId"]

    return target_ami


def get_subnet_id(boto3client: botocore.client, subnet_name: str):
    """
    Search for a subnet with a specified name, and, if one is found, return the subnet id.

    Args:
        boto3client (botocore.client): The Boto3 client to use to talk to the Amazon E2 service
        subnet_name (str): The name of the subnet to search for

    Returns:
        On success:
            subnet_id (str): The ID of the subnet with the specified name
        On failure:
            empty string
    """
    try:
        subnets = boto3client.describe_subnets()
    except botocore.exceptions.ClientError as e:
        print(e)
        printred(
            f"Caught Boto3 client exception. Could not find Subnet ID for subnet '{subnet_name}'"
        )
        return ""
    desired_subnet_tag = dict(
        {
            "Key": "Name",
            "Value": subnet_name,
        }
    )
    if "Subnets" not in subnets:
        printred(f"Could not find Subnet ID for subnet '{subnet_name}'")
        return ""

    subnet_ids = list(
        filter(
            lambda subnet: "Tags" in subnet and desired_subnet_tag in subnet["Tags"],
            subnets["Subnets"],
        )
    )
    if len(subnet_ids) == 0:
        printred(f"Could not find Subnet ID for subnet '{subnet_name}'")
        return ""
    elif len(subnet_ids) > 1:
        printyellow(
            f"Warning, found {len(subnet_ids)} matching subnet ids for subnet {subnet_name}. Using the first one"
        )

    return subnet_ids[0]["SubnetId"]


def get_security_group_id(boto3client: botocore.client, security_group_name: str):
    """
    Search for a security group with a specified name, and, if one is found, return the security group id.

    Args:
        boto3client (botocore.client): The Boto3 client to use to talk to the Amazon E2 service
        security_group_name (str): The name of the security group to search for

    Returns:
        On success:
            security_group_id (str): The ID of the security group with the specified name
        On failure:
            empty string
    """
    try:
        security_groups = boto3client.describe_security_groups()
    except botocore.exceptions.ClientError as e:
        print(e)
        printred(
            f"Caught Boto3 client exception. Could not find security group id for security group '{security_group_name}'"
        )
        return ""
    desired_sec_group_tag = dict({"Key": "Name", "Value": security_group_name})
    if "SecurityGroups" not in security_groups:
        printred(f"Could not find security group ID for security group '{security_group_name}'")
        return ""

    sec_groups_ids = list(
        filter(
            lambda sec_group: "Tags" in sec_group and desired_sec_group_tag in sec_group["Tags"],
            security_groups["SecurityGroups"],
        )
    )
    if len(sec_groups_ids) == 0:
        printred(f"Could not find security group ID for security group '{security_group_name}'")
        return ""
    elif len(sec_groups_ids) > 1:
        printyellow(
            f"Warning, found {len(sec_groups_ids)} matching security group ids for {security_group_name}. Using the first one"
        )

    return sec_groups_ids[0]["GroupId"]


def is_instance_running(
    boto3client: botocore.client,
    instance_id: str,
) -> bool:

    """
    Check if the specified instance is running. Warning: to return the correct result, you need to pass
    to this function a boto3 client set up to work in the region where the instance is also located.

    Args:
        boto3client (botocore.client): The Boto3 client to use to talk to the Amazon E2 service
        instance_id (str): The ID of the instance whose status we want to check

    Returns:
        is_running (bool): Whether the instance is 'running' or not
    """
    try:
        response = boto3client.describe_instance_status(
            InstanceIds=[instance_id], IncludeAllInstances=True
        )
    except botocore.exceptions.ClientError as e:
        printred("Got exception:")
        print(e)
        return False
    if response["InstanceStatuses"][0]["InstanceState"]["Name"] == "running":
        return True
    return False


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
        boto3client (botocore.client): The Boto3 client to use to talk to the Amazon E2 service
        region_name (str): The name of the region of interest (e.g. "us-east-1")
        instance_type (str): The type of instance to create (i.e. g4dn.xlarge)
        instance_AMI (str): The AMI to use for the instance (i.e. ami-0b9c9d7f7f8b8f8b9)
        key_name (str): The name of the AWS key to use for connecting to the instance
        disk_size (int): The size (in GB) of the additional EBS disk volume to attach

    Returns:
        instance_id (str): The ID of the created instance
    """
    branch_name = get_whist_branch_name()
    instance_name = f"{instances_name_tag}-{branch_name}"

    subnet_id = get_subnet_id(boto3client, "DefaultSubnetdev")
    security_group_id = get_security_group_id(boto3client, "MandelboxesSecurityGroupdev")

    kwargs = {
        "BlockDeviceMappings": [
            {
                "DeviceName": "/dev/sda1",
                "Ebs": {"DeleteOnTermination": True, "VolumeSize": disk_size, "VolumeType": "gp3"},
            },
        ],
        "ImageId": instance_AMI,
        "InstanceType": instance_type,  # should be g4dn.xlarge for testing the server protocol
        "MaxCount": 1,
        "MinCount": 1,
        "TagSpecifications": [
            {
                "ResourceType": "instance",
                "Tags": [
                    {"Key": "Name", "Value": instance_name},
                    {"Key": "RunID", "Value": github_run_id},
                ],
            },
        ],
        "SubnetId": subnet_id,  # (DefaultSubnetdev)
        "SecurityGroupIds": [security_group_id],  # (MandelboxesSecurityGroupdev)
        "InstanceInitiatedShutdownBehavior": "terminate",
        "KeyName": key_name,  # needs to be the same that's loaded on the client calling this function
    }

    # Create the EC2 instance
    try:
        resp = boto3client.run_instances(**kwargs)
    except botocore.exceptions.ClientError as e:
        print(e)
        printred(
            f"Caught Boto3 client exception. Could not create EC2 instance with AMI '{instance_AMI}'"
        )
        return ""

    instance_id = resp["Instances"][0]["InstanceId"]
    print(
        f"Created EC2 instance with id: {instance_id}, type={instance_type}, ami={instance_AMI}, key_name={key_name}, disk_size={disk_size}"
    )
    return instance_id


def start_instance_and_get_lock(boto3client: botocore.client, instance_id: str, ssh_key_path: str):
    """
    Start an existing instance and acquire the instance's E2E lock. If acquiring the E2E lock
    fails, sleep for lock_contention_wait_time_seconds and retry. Each time, we have to ensure
    that the instance is still running, and if not, turn it back on before attempting to acquire
    the lock. This is because other E2E actors might have turned the instance off after completing
    their work.

    Args:
        boto3client (botocore.client): The Boto3 client to use to talk to the Amazon E2 service
        instance_id (str): The ID of the instance to start and for which we want to acquire the lock
        ssh_key_path (str): The path to the SSH key to be used to access the instance via SSH

    Returns:
        success (bool): indicates whether the start/locking succeeded.
    """
    # Write name of the lock to file to allow for unlocking in case of crash
    with open("lock_name.txt", "w+") as lock_log_file:
        lock_log_file.write(f"{unique_lock_path}\n")

    deadline = time.time() + lock_get_attempt_timeout_seconds
    while True:
        if time.time() > deadline:
            break
        if not is_instance_running(boto3client, instance_id):
            print(f"Instance {instance_id} is currently not running. Attempting to start it...")
            result = start_instance(boto3client, instance_id)
            if not result:
                return False
        # Get IP address
        ip_addresses = get_instance_ip(boto3client, instance_id)
        public_ip = ip_addresses[0]["public"]
        # Attempt to get the lock
        if attempt_request_lock(public_ip, ssh_key_path):
            print(f"Successfully acquired lock on instance {instance_id}!")
            return True
        printgrey(
            f"Failed to get lock on {instance_id}! Waiting for {lock_contention_wait_time_seconds}s and retrying..."
        )
        time.sleep(lock_contention_wait_time_seconds)

    printyellow(
        f"Could not acquire lock on instance {instance_id} after waiting for {lock_get_attempt_timeout_seconds}s! Giving up."
    )
    return False


def start_instance(
    boto3client: botocore.client, instance_id: str, max_retries: int = start_instance_retries
) -> bool:
    """
    Attempt to turn on an existing EC2 instance. Return a bool indicating whether the operation succeeded.

    Args:
        boto3client (botocore.client): The Boto3 client to use to talk to the Amazon E2 service
        instance_id (str): The ID of the instance to start

    Returns:
        success (bool): indicates whether the start succeeded.
    """
    for retry in range(max_retries):
        try:
            response = boto3client.start_instances(InstanceIds=[instance_id], DryRun=False)
            print(response)
        except botocore.exceptions.ClientError as e:
            print(
                f"Could not start instance (retry {retry + 1}/{max_retries}). Caught exception: {e}"
            )
            if (
                e.response["Error"]["Code"] == "IncorrectInstanceState"
                or e.response["Error"]["Code"] == "IncorrectSpotRequestState"
            ) and retry < max_retries - 1:
                time.sleep(60)
                continue
            else:
                printred(f"Could not start instance after {max_retries} retries. Giving up now.")
                return False
        break
    return True


def stop_instance_and_release_lock(
    boto3client: botocore.client, instance_id: str, ssh_key_path: str
):
    """
    Stop an existing instance and release the instance's E2E lock. If releasing the E2E lock
    fails, sleep for lock_contention_wait_time_seconds and retry.

    Args:
        boto3client (botocore.client): The Boto3 client to use to talk to the Amazon E2 service
        instance_id (str): The ID of the instance to start and for which we want to acquire the lock
        ssh_key_path (str): The path to the SSH key to be used to access the instance via SSH

    Returns:
        success (bool): indicates whether the stop and unlocking both succeeded.
    """
    # Write name of the lock to file to allow for unlocking in case of crash
    with open("lock_name.txt", "w+") as lock_log_file:
        lock_log_file.write(f"{unique_lock_path}\n")

    # Safety checks
    if not is_instance_running(boto3client, instance_id):
        printyellow(
            "Warning, the instance was already stopped when attempting to stop it and release the lock."
        )
        return False

    # Unlocking
    unlocking_succeded = True
    ip_addresses = get_instance_ip(boto3client, instance_id)
    public_ip = ip_addresses[0]["public"]
    # Attempt to release the lock
    for retry in range(UNLOCKING_MAX_RETRIES):
        print(
            f"Attempting to release E2E lock on instance {instance_id} (retry {retry+1}/{UNLOCKING_MAX_RETRIES})..."
        )
        if attempt_release_lock(public_ip, ssh_key_path):
            print(f"Successfully released E2E lock on instance {instance_id}!")
            break
        if retry == UNLOCKING_MAX_RETRIES - 1:
            printred(
                f"Unable to release E2E lock after {UNLOCKING_MAX_RETRIES} retries! Giving up!"
            )
            unlocking_succeded = False

        time.sleep(lock_contention_wait_time_seconds)

    result = stop_instance(boto3client, instance_id)

    return result and unlocking_succeded


def stop_instance(boto3client: botocore.client, instance_id: str) -> bool:
    """
    Attempt to turn off an existing EC2 instance. Return a bool indicating whether the operation succeeded.

    Args:
        boto3client (botocore.client): The Boto3 client to use to talk to the Amazon E2 service
        instance_id (str): The ID of the instance to stop

    Returns:
        success (bool): indicates whether the stop succeeded.
    """
    try:
        response = boto3client.stop_instances(InstanceIds=[instance_id], DryRun=False)
        print(response)
    except botocore.exceptions.ClientError as e:
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
        boto3client (botocore.client): The Boto3 client to use to talk to the Amazon E2 service
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
    Get the public and private IP addresses of an existing EC2 instance.

    Args:
        boto3client (botocore.client): The Boto3 client to use to talk to the Amazon EC2 service
        instance_id (str): The ID of the instance of interest

    Returns:
        retval (list): A list of dictionaries with the public and private IPs of every instance
                       with instance id equal to the parameter.
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


def create_or_start_aws_instance(
    boto3client, region_name, existing_instance_id, ssh_key_name, lock_needed, ssh_key_path
):
    """
    Connect to an existing instance (if the parameter existing_instance_id is not empty) or create a new one

    Args:
        boto3client (botocore.client): The Boto3 client to use to talk to the Amazon E2 service
        region_name (str): The name of the region of interest (e.g. "us-east-1")
        existing_instance_id (str): The ID of the instance to connect to, or "" to create a new one
        ssh_key_name (str): The name of the AWS key to use to create a new instance. This parameter is
                            ignored if a valid instance ID is passed to the existing_instance_id parameter.

    Returns:
        instance_id (str): the ID of the started instance. This can be the existing instance
                           (if we passed a existing_instance_id) or the new instance
                           (if we passed an empty string to existing_instance_id)
    """
    # Attempt to start existing instance
    if existing_instance_id != "":
        instance_id = existing_instance_id
        result = start_instance_and_get_lock(boto3client, instance_id, lock_needed, ssh_key_path)
        if result is True:
            # Wait for the instance to be running
            wait_for_instance_to_start_or_stop(boto3client, instance_id, stopping=False)
            return instance_id

    # Define the AWS machine variables

    # The base AWS-provided AMI we build our AMI from: AWS Ubuntu Server 20.04 LTS
    instance_AMI = get_current_AMI(boto3client, region_name)
    instance_type = INSTANCE_TYPE  # The type of instance we want to create

    print(f"Creating AWS EC2 instance of size: {instance_type} and with AMI: {instance_AMI}...")

    # Create our EC2 instance
    instance_id = create_ec2_instance(
        boto3client=boto3client,
        region_name=region_name,
        instance_type=instance_type,
        instance_AMI=instance_AMI,
        key_name=ssh_key_name,
        disk_size=64,  # GB
    )
    if instance_id == "":
        print("Creating instance failed, so we don't wait for it to start")
        return ""

    # Give a little time for the instance to be recognized in AWS
    time.sleep(5)

    # Wait for the instance to be running
    wait_for_instance_to_start_or_stop(boto3client, instance_id, stopping=False)

    if not start_instance_and_get_lock(boto3client, instance_id, lock_needed, ssh_key_path):
        return False

    return instance_id


def get_client_and_instances(
    region_name,
    ssh_key_name,
    use_two_instances,
    existing_server_instance_id,
    existing_client_instance_id,
):
    """
    Get a Boto3 client and start/create instances in a given AWS region

    Args:
        region_name (str): The name of the region of interest (e.g. "us-east-1")
        ssh_key_name (str): The name of the AWS key to use for connecting to the instance(s)
        use_two_instances (bool): A boolean indicating whether we are running the E2E test using one or two instances
        existing_server_instance_id (str): The ID of an existing instance to reuse for the server in the E2E test
        existing_client_instance_id (str): The ID of an existing instance to reuse for the client in the E2E test

    Returns:
        On success:
            boto3client (botocore.client): The Boto3 client to use to talk to the Amazon E2 service
            server_instance_id (str): The ID of the AWS instance to be used for the server
            client_instance_id (str): The ID of the AWS instance to be used for the client
        On failure:
            None
    """
    boto3client = boto3.client("ec2", region_name=region_name)

    server_instance_id = create_or_start_aws_instance(
        boto3client,
        region_name,
        existing_server_instance_id,
        ssh_key_name,
    )
    if server_instance_id == "":
        printyellow(f"Creating new instance on {region_name} for the server failed!")
        return None

    client_instance_id = (
        create_or_start_aws_instance(
            boto3client,
            region_name,
            existing_client_instance_id,
            ssh_key_name,
        )
        if use_two_instances
        else server_instance_id
    )
    if client_instance_id == "":
        printyellow(f"Creating/starting new instance on {region_name} for the client failed!")
        # Terminate or stop server AWS instance
        terminate_or_stop_aws_instance(
            boto3client, server_instance_id, server_instance_id != existing_server_instance_id
        )
        return None

    return boto3client, server_instance_id, client_instance_id


def terminate_or_stop_aws_instance(boto3client, instance_id, should_terminate):
    """
    Stop (if should_terminate==False) or terminate (if should_terminate==True) a AWS instance

    Args:
        boto3client (botocore.client): The Boto3 client to use to talk to the Amazon E2 service
        instance_id (str): The ID of the instance to stop or terminate
        should_terminate (bool): A boolean indicating whether the instance should be terminated (instead of stopped)

    Returns:
        None
    """
    if should_terminate:
        # Terminating the instance and waiting for them to shutdown
        print(f"Testing complete, terminating EC2 instance")
        try:
            boto3client.terminate_instances(InstanceIds=[instance_id])
        except botocore.exceptions.ClientError as e:
            printred(f"Caught Boto3 client exception while terminating instance {instance_id}!")
            print(e)
            return
    else:
        # Stopping the instance and waiting for it to shutdown
        print(f"Testing complete, stopping EC2 instance")
        result = False
        try:
            result = stop_instance(boto3client, instance_id)
        except botocore.exceptions.ClientError as e:
            printred(f"Caught Boto3 client exception while terminating instance {instance_id}!")
            print(e)
        if result is False:
            printyellow("Error while stopping the EC2 instance!")
            return

    # Wait for the instance to be terminated
    wait_for_instance_to_start_or_stop(boto3client, instance_id, stopping=True)
