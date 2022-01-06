#!/usr/bin/env python3

import sys
import os
import time
import argparse
import subprocess
import pexpect
import json
import multiprocessing

# Get tools to create, destroy and manage AWS instances
from e2e_helpers.aws_tools import (
    get_boto3client,
    create_or_start_aws_instance,
    get_instance_ip,
    terminate_or_stop_aws_instance,
)

# Get tools to run operations on a dev instance via SSH
from e2e_helpers.dev_instance_tools import (
    attempt_ssh_connection,
    wait_until_cmd_done,
)

# Get tools to programmatically run Whist components on a remote machine
from e2e_helpers.whist_remote import (
    server_setup_process,
    client_setup_process,
    start_host_service_on_instance,
    run_server_on_instance,
    run_client_on_instance,
)

# Get tools to programmatically run Whist components on a remote machine
from e2e_helpers.remote_exp_tools import (
    extract_server_logs_from_instance,
    extract_client_logs_from_instance,
)

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))

DESCRIPTION = """
This script will spin a g4dn.2xlarge EC2 instance with two Docker containers
and run a protocol streaming performance test between the two of them. 
"""

parser = argparse.ArgumentParser(description=DESCRIPTION)

parser.add_argument(
    "--ssh-key-name",
    help="The name of the AWS RSA SSH key you wish to use to connect to the instances for configuration. This is most likely the name of your personal development keypair in AWS. Required.",
    required=True,
)
parser.add_argument(
    "--ssh-key-path",
    help="The full path to your RSA SSH key corresponding to the --ssh-key-name you provided. Required.",
    required=True,
)
parser.add_argument(
    "--github-token",
    help="The GitHub Personal Access Token with permission to fetch the whisthq/whist repository. Required.",
    required=True,
)

parser.add_argument(
    "--region-name",
    help="The AWS region to use for testing",
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
    default="us-east-1",
)

parser.add_argument(
    "--use-existing-server-instance",
    help="The ID of the existing instance to use for the test. If left empty, a clean instance will be generated instead.",
    type=str,
    default="",
)

parser.add_argument(
    "--use-existing-client-instance",
    help="The ID of the existing instance to use for the test. If left empty, a clean instance will be generated instead.",
    type=str,
    default="",
)

parser.add_argument(
    "--use-two-instances",
    help="Whether to run the client on a separate AWS instance, instead of the same as the server.",
    type=str,
    choices=["false", "true"],
    default="false",
)

parser.add_argument(
    "--testing-url",
    help="The URL to visit for testing",
    type=str,
    default="https://fractal-test-assets.s3.amazonaws.com/SpaceX+Launches+4K+Demo.mp4",
)

parser.add_argument(
    "--testing-time",
    help="The length of the performance test in seconds",
    type=int,
    default=126,  # Length of the video in link above is 2mins, 6seconds
)

parser.add_argument(
    "--cmake-build-type",
    help="The cmake build type to use",
    type=str,
    choices=["dev", "prod", "metrics"],
    default="metrics",
)

parser.add_argument(
    "--aws-credentials-filepath",
    help="The path to the file containing the AWS config credentials",
    type=str,
    default="~/.aws/credentials",
)

args = parser.parse_args()



# This main loop creates two AWS EC2 instances, one client, one server, and sets up
# a protocol streaming test between them
if __name__ == "__main__":
    # Retrieve args
    ssh_key_name = args.ssh_key_name  # In CI, this is "protocol_performance_testing_sshkey"
    ssh_key_path = args.ssh_key_path
    github_token = args.github_token  # The PAT allowing us to fetch code from GitHub
    testing_url = args.testing_url
    testing_time = args.testing_time
    region_name = args.region_name
    use_two_instances = True if args.use_two_instances == "true" else False

    # Load the SSH key
    if not os.path.isfile(ssh_key_path):
        print("SSH key file {} does not exist".format(ssh_key_path))
        exit()

    # Load the AWS credentials path
    aws_credentials_filepath = args.aws_credentials_filepath

    # Create a boto3 client, create or start the instance(s).
    boto3client = get_boto3client(region_name)
    server_instance_id = create_or_start_aws_instance(
        boto3client, region_name, args.use_existing_server_instance, ssh_key_name
    )
    client_instance_id = (
        create_or_start_aws_instance(
            boto3client, region_name, args.use_existing_client_instance, ssh_key_name
        )
        if use_two_instances
        else server_instance_id
    )

    # Get the IP address of the instance(s)
    server_instance_ip = get_instance_ip(boto3client, server_instance_id)
    server_hostname = server_instance_ip[0]["public"]
    server_private_ip = server_instance_ip[0]["private"]
    client_instance_ip = (
        get_instance_ip(boto3client, client_instance_id)
        if use_two_instances
        else server_instance_ip
    )
    client_hostname = client_instance_ip[0]["public"]
    client_private_ip = client_instance_ip[0]["private"]
    if not use_two_instances:
        print("Connected to server/client AWS instance with hostname: {}!".format(server_hostname))
    else:
        print(
            "Connected to server AWS instance with hostname: {} and client AWS instance with hostname: {}!".format(
                server_hostname, client_hostname
            )
        )

    username = "ubuntu"
    pexpect_prompt_server = "{}@ip-{}".format(username, server_private_ip.replace(".", "-"))
    pexpect_prompt_client = (
        "{}@ip-{}".format(username, client_private_ip.replace(".", "-"))
        if use_two_instances
        else pexpect_prompt_server
    )
    aws_timeout = 1200  # 10 mins is not enough to build the base mandelbox, so we'll go ahead with 20 mins to be safe

    # Create local folder for logs
    perf_logs_folder_name = time.strftime("%Y_%m_%d@%H-%M-%S")
    perf_logs_folder_name = "./perf_logs/{}".format(perf_logs_folder_name)
    command = "mkdir -p {}/server {}/client".format(perf_logs_folder_name, perf_logs_folder_name)
    local_process = pexpect.spawn(command, timeout=aws_timeout)
    local_process.expect(["\$", "%", pexpect.EOF])
    local_process.kill(0)

    server_log_filepath = "{}/server_monitoring_log.txt".format(perf_logs_folder_name)
    client_log_filepath = "{}/client_monitoring_log.txt".format(perf_logs_folder_name)

    manager = multiprocessing.Manager()
    args_dict = manager.dict()
    args_dict["username"] = username
    args_dict["server_hostname"] = server_hostname
    args_dict["client_hostname"] = client_hostname
    args_dict["ssh_key_path"] = ssh_key_path
    args_dict["aws_timeout"] = aws_timeout
    args_dict["server_log_filepath"] = server_log_filepath
    args_dict["client_log_filepath"] = client_log_filepath
    args_dict["pexpect_prompt_server"] = pexpect_prompt_server
    args_dict["pexpect_prompt_client"] = pexpect_prompt_client
    args_dict["github_token"] = github_token
    args_dict["use_two_instances"] = use_two_instances
    args_dict["testing_time"] = testing_time
    args_dict["aws_credentials_filepath"] = aws_credentials_filepath
    args_dict["cmake_build_type"] = args.cmake_build_type

    # If using two instances, parallelize the host-setup and building of the docker containers to save time
    p1 = multiprocessing.Process(target=server_setup_process, args=[args_dict])
    p2 = multiprocessing.Process(target=client_setup_process, args=[args_dict])
    if use_two_instances:
        p1.start()
        p2.start()
        p1.join()
        p2.join()
    else:
        p1.start()
        p1.join()
        p2.start()
        p2.join()

    server_log = open("{}/server_monitoring_log.txt".format(perf_logs_folder_name), "a")
    client_log = open("{}/client_monitoring_log.txt".format(perf_logs_folder_name), "a")
    server_cmd = "ssh {}@{} -i {}".format(username, server_hostname, ssh_key_path)
    client_cmd = "ssh {}@{} -i {}".format(username, client_hostname, ssh_key_path)

    server_hs_process = attempt_ssh_connection(
        server_cmd, aws_timeout, server_log, pexpect_prompt_server, 5
    )
    client_hs_process = (
        attempt_ssh_connection(client_cmd, aws_timeout, client_log, pexpect_prompt_client, 5)
        if use_two_instances
        else server_hs_process
    )

    # Build and run host-service on server
    start_host_service_on_instance(server_hs_process)

    if use_two_instances:
        # Build and run host-service on server
        start_host_service_on_instance(client_hs_process)

    server_pexpect_process = attempt_ssh_connection(
        server_cmd, aws_timeout, server_log, pexpect_prompt_server, 5
    )

    client_pexpect_process = attempt_ssh_connection(
        client_cmd, aws_timeout, client_log, pexpect_prompt_client, 5
    )

    # 5- Run the protocol server, and retrieve the connection configs
    server_docker_id, json_data = run_server_on_instance(server_pexpect_process)
    json_data["initial_url"] = testing_url

    # Run the dev client
    client_docker_id = run_client_on_instance(client_pexpect_process, json_data)

    # Wait <testing_time> seconds to generate enough data
    time.sleep(testing_time)

    # Extract the client/server perf logs from the two docker containers
    print("Initiating LOG GRABBING ssh connection(s) with the AWS instance(s)...")

    log_grabber_server_process = attempt_ssh_connection(
        server_cmd, aws_timeout, server_log, pexpect_prompt_server, 5
    )
    log_grabber_server_process.expect(pexpect_prompt_server)

    log_grabber_client_process = log_grabber_server_process
    if use_two_instances:
        log_grabber_client_process = attempt_ssh_connection(
            client_cmd, aws_timeout, client_log, pexpect_prompt_client, 5
        )
        log_grabber_client_process.expect(pexpect_prompt_client)

    extract_server_logs_from_instance(
        log_grabber_server_process,
        pexpect_prompt_server,
        server_docker_id,
        ssh_key_path,
        username,
        server_hostname,
        aws_timeout,
        perf_logs_folder_name,
        server_log,
    )
    extract_client_logs_from_instance(
        log_grabber_client_process,
        pexpect_prompt_client,
        client_docker_id,
        ssh_key_path,
        username,
        client_hostname,
        aws_timeout,
        perf_logs_folder_name,
        client_log,
    )

    # Clean up the instance(s)
    # Exit the server/client mandelboxes
    server_pexpect_process.sendline("exit")
    wait_until_cmd_done(server_pexpect_process, pexpect_prompt_server)
    client_pexpect_process.sendline("exit")
    wait_until_cmd_done(client_pexpect_process, pexpect_prompt_client)

    # Delete all Docker containers

    command = "docker stop $(docker ps -aq) && docker rm $(docker ps -aq)"
    server_pexpect_process.sendline(command)
    wait_until_cmd_done(server_pexpect_process, pexpect_prompt_server)
    if use_two_instances:
        client_pexpect_process.sendline(command)
        wait_until_cmd_done(client_pexpect_process, pexpect_prompt_client)

    # Terminate the host service
    server_hs_process.sendcontrol("c")
    server_hs_process.kill(0)
    if use_two_instances:
        client_hs_process.sendcontrol("c")
        client_hs_process.kill(0)

    # Terminate all pexpect processes
    # hs_process.kill(0)
    server_pexpect_process.kill(0)
    client_pexpect_process.kill(0)
    log_grabber_server_process.kill(0)
    if use_two_instances:
        log_grabber_client_process.kill(0)

    # Close the log files
    server_log.close()
    client_log.close()

    # Terminate or stop AWS instance(s)
    terminate_or_stop_aws_instance(
        boto3client, server_instance_id, server_instance_id != args.use_existing_server_instance
    )
    if use_two_instances:
        terminate_or_stop_aws_instance(
            boto3client, client_instance_id, client_instance_id != args.use_existing_client_instance
        )

    print("Instance successfully stopped/terminated, goodbye")

    print("Done")
