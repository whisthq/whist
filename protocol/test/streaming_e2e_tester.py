#!/usr/bin/env python3

import sys
import os
import time
import argparse
import subprocess
import pexpect
import json
import multiprocessing
import platform

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
    get_whist_branch_name,
)

# Get tools to programmatically run Whist components on a remote machine
from e2e_helpers.whist_remote import (
    server_setup_process,
    client_setup_process,
    start_host_service_on_instance,
    run_server_on_instance,
    run_client_on_instance,
    setup_network_conditions_client,
    restore_network_conditions_client,
    shutdown_and_wait_server_exit,
)

# Get tools to programmatically run Whist components on a remote machine
from e2e_helpers.remote_exp_tools import (
    extract_server_logs_from_instance,
    extract_client_logs_from_instance,
)

# Get tools to run commands on local machine
from e2e_helpers.local_tools import (
    get_whist_branch_name,
    get_whist_github_sha,
)

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))

DESCRIPTION = """
This script will spin a g4dn.xlarge EC2 instance with two Docker containers
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
    "--leave-instances-on",
    help="This option allows you to avoid stopping/terminating the instances upon completion of the test",
    type=str,
    choices=["false", "true"],
    default="false",
)

parser.add_argument(
    "--skip-host-setup",
    help="This option allows you to skip the host-setup on the instances to be used for the test",
    type=str,
    choices=["false", "true"],
    default="false",
)

parser.add_argument(
    "--skip-git-clone",
    help="This option allows you to skip cloning the Whist repository on the instances to be used for the test",
    type=str,
    choices=["false", "true"],
    default="false",
)


parser.add_argument(
    "--simulate-scrolling",
    help="Whether to simulate mouse scrolling on the client side",
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

parser.add_argument(
    "--network-conditions",
    help="The network condition for the experiment. The input is in the form of three comma-separated floats indicating the max bandwidth, delay (in ms), and percentage of packet drops (in the range [0.0,1.0]). 'normal' will allow the network to run with no degradation.",
    type=str,
    default="normal",
)

args = parser.parse_args()


# This main loop creates two AWS EC2 instances, one client, one server, and sets up
# a protocol streaming test between them
if __name__ == "__main__":

    # Create local folder for logs
    experiment_start_time = time.strftime("%Y_%m_%d@%H-%M-%S")
    perf_logs_folder_name = os.path.join("perf_logs", experiment_start_time)
    os.makedirs(os.path.join(perf_logs_folder_name, "server"))
    os.makedirs(os.path.join(perf_logs_folder_name, "client"))

    # Retrieve args
    ssh_key_name = args.ssh_key_name  # In CI, this is "protocol_performance_testing_sshkey"
    ssh_key_path = args.ssh_key_path
    github_token = args.github_token  # The PAT allowing us to fetch code from GitHub
    running_in_ci = os.getenv("CI")
    if running_in_ci is None or running_in_ci == "false":
        running_in_ci = False
    else:
        running_in_ci = True
    testing_url = args.testing_url
    testing_time = args.testing_time
    region_name = args.region_name
    use_two_instances = True if args.use_two_instances == "true" else False
    simulate_scrolling = True if args.simulate_scrolling == "true" else False

    use_existing_client_instance = args.use_existing_client_instance
    use_existing_server_instance = args.use_existing_server_instance

    if use_existing_client_instance != "" and use_two_instances:
        print(
            "Error: the `use-two-instances` flag is set to `false` but a non-empty instance ID was passed with the `use-existing-client-instance` flag."
        )
        sys.exit(-1)

    skip_git_clone = args.skip_git_clone
    skip_host_setup = args.skip_host_setup

    network_conditions = args.network_conditions

    # Load the SSH key
    if not os.path.isfile(ssh_key_path):
        print("SSH key file {} does not exist".format(ssh_key_path))
        exit()

    # Load the AWS credentials path
    aws_credentials_filepath = args.aws_credentials_filepath

    # Create a boto3 client, create or start the instance(s).
    boto3client = get_boto3client(region_name)
    server_instance_id = create_or_start_aws_instance(
        boto3client, region_name, use_existing_server_instance, ssh_key_name, running_in_ci
    )
    client_instance_id = (
        create_or_start_aws_instance(
            boto3client, region_name, use_existing_client_instance, ssh_key_name, running_in_ci
        )
        if use_two_instances
        else server_instance_id
    )

    leave_instances_on = args.leave_instances_on

    # Save to 'instances_to_clean.txt' file names of new instances that were created. These will have to be terminated
    # by a successive Github action in case this script crashes before being able to terminate them itself.
    instances_to_be_terminated = []
    instances_to_be_stopped = []
    if server_instance_id != use_existing_server_instance:
        instances_to_be_terminated.append(server_instance_id)
        # Turning off skipping git clone and host setup if we created a new instance
        skip_git_clone = "false"
        skip_host_setup = "false"
        # If we didn't intend to get a new instance, then turn off option to leave instance off
        if use_existing_server_instance != "":
            leave_instances_on = "false"
    else:
        instances_to_be_stopped.append(server_instance_id)
    if client_instance_id != server_instance_id:
        if client_instance_id != use_existing_client_instance:
            instances_to_be_terminated.append(client_instance_id)
            # Turning off skipping git clone and host setup if we created a new instance
            skip_git_clone = "false"
            skip_host_setup = "false"
            # If we didn't intend to get a new instance, then turn off option to leave instance off
            if use_existing_client_instance != "":
                leave_instances_on = "false"
        else:
            instances_to_be_stopped.append(client_instance_id)
    instances_file = open("instances_to_clean.txt", "a+")
    for i in instances_to_be_terminated:
        instances_file.write("terminate {} {}\n".format(region_name, i))
    for i in instances_to_be_stopped:
        instances_file.write("stop {} {}\n".format(region_name, i))
    instances_file.close()

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

    experiment_metadata = {
        "start_time": experiment_start_time + " local time"
        if not running_in_ci
        else experiment_start_time + " UTC",
        "network_conditions": network_conditions,
        "using_two_instances": use_two_instances,
        "branch_name": get_whist_branch_name(running_in_ci),
        "github_sha": get_whist_github_sha(running_in_ci),
        "server_hang_detected": False,
    }
    metadata_filename = os.path.join(perf_logs_folder_name, "experiment_metadata.json")
    with open(metadata_filename, "w") as metadata_file:
        json.dump(experiment_metadata, metadata_file)

    server_log_filepath = os.path.join(perf_logs_folder_name, "server_monitoring_log.txt")
    client_log_filepath = os.path.join(perf_logs_folder_name, "client_monitoring_log.txt")

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
    args_dict["running_in_ci"] = running_in_ci
    args_dict["skip_git_clone"] = skip_git_clone
    args_dict["skip_host_setup"] = skip_host_setup

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

    # Set up the network degradation conditions
    setup_network_conditions_client(
        client_pexpect_process, pexpect_prompt_client, network_conditions, running_in_ci
    )
    # Run the dev client
    client_docker_id = run_client_on_instance(client_pexpect_process, json_data, simulate_scrolling)

    # Wait <testing_time> seconds to generate enough data
    time.sleep(testing_time)

    # Restore un-degradated network conditions in case the instance is reused later on. Do this before downloading the logs to prevent the donwload from taking a long time.
    if network_conditions != "normal":
        # Get new SSH connection because current ones are connected to the mandelboxes' bash, and we cannot exit them until we have copied over the logs
        client_restore_net_process = attempt_ssh_connection(
            client_cmd, aws_timeout, client_log, pexpect_prompt_client, 5
        )
        restore_network_conditions_client(
            client_restore_net_process, pexpect_prompt_client, running_in_ci
        )
        client_restore_net_process.kill(0)

    # Extract the client/server perf logs from the two docker containers
    print("Initiating LOG GRABBING ssh connection(s) with the AWS instance(s)...")

    log_grabber_server_process = attempt_ssh_connection(
        server_cmd, aws_timeout, server_log, pexpect_prompt_server, 5
    )
    if not running_in_ci:
        log_grabber_server_process.expect(pexpect_prompt_server)

    log_grabber_client_process = log_grabber_server_process
    if use_two_instances:
        log_grabber_client_process = attempt_ssh_connection(
            client_cmd, aws_timeout, client_log, pexpect_prompt_client, 5
        )
        if not running_in_ci:
            log_grabber_client_process.expect(pexpect_prompt_client)

    server_hang_detected = False
    if shutdown_and_wait_server_exit(
        server_pexpect_process, "Both whist-application and WhistServer have exited."
    ):
        print("Server has exited gracefully.")
    else:
        print("Server has not exited gracefully!")
        server_hang_detected = True

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
        running_in_ci,
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
        running_in_ci,
    )

    # Clean up the instance(s)

    # Exit the server/client mandelboxes
    server_pexpect_process.sendline("exit")
    wait_until_cmd_done(server_pexpect_process, pexpect_prompt_server, running_in_ci)
    client_pexpect_process.sendline("exit")
    wait_until_cmd_done(client_pexpect_process, pexpect_prompt_client, running_in_ci)

    # Delete all Docker containers

    command = "docker stop $(docker ps -aq) && docker rm $(docker ps -aq)"
    server_pexpect_process.sendline(command)
    wait_until_cmd_done(server_pexpect_process, pexpect_prompt_server, running_in_ci)
    if use_two_instances:
        client_pexpect_process.sendline(command)
        wait_until_cmd_done(client_pexpect_process, pexpect_prompt_client, running_in_ci)

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

    if leave_instances_on == "false":
        # Terminate or stop AWS instance(s)
        terminate_or_stop_aws_instance(
            boto3client, server_instance_id, server_instance_id != use_existing_server_instance
        )
        if use_two_instances:
            terminate_or_stop_aws_instance(
                boto3client,
                client_instance_id,
                client_instance_id != use_existing_client_instance,
            )
    else:
        # Save instance IDs to file for reuse by later runs
        with open("instances_left_on.txt", "w") as instances_file:
            instances_file.write("{}\n".format(server_instance_id))
            if client_instance_id != server_instance_id:
                instances_file.write("{}\n".format(client_instance_id))

    print("Instance successfully stopped/terminated, goodbye")

    # No longer need the new_instances.txt file because the script has already terminated (if needed) the instances itself
    os.remove("instances_to_clean.txt")

    if server_hang_detected:
        print("Exiting with failure due to server hang!")

        # Update experiment metadata
        experiment_metadata["server_hang_detected"] = server_hang_detected
        with open(metadata_filename, "w") as metadata_file:
            json.dump(experiment_metadata, metadata_file)

        exit(-1)
    else:
        print("Done")
