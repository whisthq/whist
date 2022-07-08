#!/usr/bin/env python3

import argparse, boto3, os

from helpers.common.ssh_tools import (
    attempt_ssh_connection,
    force_unlock,
)
from helpers.aws.boto3_tools import terminate_or_stop_aws_instance, get_instance_ip
from helpers.common.constants import (
    username,
)
from helpers.aws.boto3_tools import (
    is_instance_running,
)

DESCRIPTION = "This script will allow you to force unlock instances in case a E2E runner erroneously exits while holding a lock"
parser = argparse.ArgumentParser(description=DESCRIPTION)

parser.add_argument(
    "--ssh-key-path",
    help="The path to the .pem certificate on this machine to use in connection to the RSA SSH key passed to \
    the --ssh-key-name arg. Required.",
    required=True,
)

parser.add_argument(
    "--region-name",
    help="The AWS region where the instance(s) to unlock are based",
    type=str,
    choices=[
        "us-east-1",
        "us-east-2",
        "us-west-1",
        "us-west-2",
        "af-south-1",
        "ap-east-1",
        "ap-south-1",
        "ap-northeast-3",
        "ap-northeast-2",
        "ap-southeast-1",
        "ap-southeast-2",
        "ap-southeast-3",
        "ap-northeast-1",
        "ca-central-1",
        "eu-central-1",
        "eu-west-1",
        "eu-west-2",
        "eu-south-1",
        "eu-west-3",
        "eu-north-1",
        "sa-east-1",
    ],
    required=True,
)

parser.add_argument(
    "instances", metavar="instances", help="The IDs of the instances to unlock", type=str, nargs="+"
)

parser.add_argument(
    "--stop-instances",
    help="Whether to stop the instances after force unlocking",
    action="store_true",
)

args = parser.parse_args()

if __name__ == "__main__":

    boto3client = boto3.client("ec2", region_name=args.region_name)
    log_filepath = os.path.join(".", "perf_logs", "force_unlock.log")
    if not os.path.isdir(os.path.join(".", "perf_logs")):
        print(f"Error, cannot create {log_filepath} file!")
    force_unlock_logfile = open(log_filepath, "w+")

    for instance_id in args.instances:
        if is_instance_running(boto3client, instance_id) == False:
            print(f"Cannot unlock instance `{instance_id}` because it is not running!")
            continue
        instance_ips = get_instance_ip(boto3client, instance_id)
        public_ip = instance_ips[0]["public"]
        private_ip = instance_ips[0]["private"].replace(".", "-")
        ssh_cmd = f"ssh {username}@{public_ip} -i {args.ssh_key_path} -o TCPKeepAlive=yes -o ServerAliveInterval=15"
        pexpect_prompt = f"{username}@ip-{private_ip}"
        pexpect_process = attempt_ssh_connection(ssh_cmd, force_unlock_logfile, pexpect_prompt)

        unlock_max_retries = 5
        for retry in range(unlock_max_retries):
            print(
                f"Attempting to force unlock instance `{instance_id}` (retry {retry+1}/{unlock_max_retries})..."
            )
            result = force_unlock(pexpect_process, pexpect_prompt)
            if result:
                print(f"Succesfully force unlocked instance `{instance_id}`!")
                break
            print(f"Failed to force unlock instance `{instance_id}`")
            if retry == unlock_max_retries - 1:
                print(f"Giving up unlocking `{instance_id}`")

        if args.stop_instances:
            terminate_or_stop_aws_instance(boto3client, instance_id, should_terminate=False)

        # Newline for spacing
        print()
