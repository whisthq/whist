#!/usr/bin/env python3

import sys
import os
import time
import argparse
import boto3
import paramiko
import subprocess
import pexpect
import json

from testing_helpers import (
    get_boto3client,
    create_ec2_instance,
    wait_for_instance_to_start_or_stop,
    get_instance_ip,
    start_instance,
    stop_instance,
)

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))

DESCRIPTION = """
This script will spin a g4dn.2xlarge EC2 instance with two docker containers
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
    help="The GitHub Personal Access Token with permission to fetch the fractal/fractal repository. Required.",
    required=True,
)

parser.add_argument(
    "--region_name",
    help="The AWS region to use for testing",
    type=str,
    choices=["us-east-1", "us-east-2", "us-west-1", "us-west-2"],
    default="us-east-1",
)

parser.add_argument(
    "--use-existing-instance",
    help="The ID of the existing instance to use for the test. If left empty, a clean instance will be generated instead.",
    type=str,
    default="",
)

parser.add_argument(
    "--testing_url",
    help="The URL to visit for testing",
    type=str,
    default="https://fractal-test-assets.s3.amazonaws.com/SpaceX+Launches+4K+Demo.mp4",
)

parser.add_argument(
    "--testing_time",
    help="The length of the perf test in seconds",
    type=int,
    default=126,  # Length of the video in link above is 2mins, 6seconds
)

parser.add_argument(
    "--cmake_build_type",
    help="The cmake build type to use",
    type=str,
    choices=["dev", "prod", "perf"],
    default="perf",
)

args = parser.parse_args()

# Ubuntu Server Instance IDs for the 4 most popular AWS regions
instance_AMI_dict = {}
instance_AMI_dict["us-east-1"] = "ami-0885b1f6bd170450c"
instance_AMI_dict["us-east-2"] = "ami-0629230e074c580f2"
instance_AMI_dict["us-west-1"] = "ami-053ac55bdcfe96e85"
instance_AMI_dict["us-west-2"] = "ami-036d46416a34a611c"


def attempt_ssh_connection(ssh_command, timeout, log_file_handle, pexpect_prompt, max_retries):
    for retries in range(max_retries):
        child = pexpect.spawn(ssh_command, timeout=timeout, logfile=log_file_handle.buffer)
        result_index = child.expect(
            [
                "Connection refused",
                "Are you sure you want to continue connecting (yes/no/[fingerprint])?",
                pexpect_prompt,
                pexpect.EOF,
                pexpect.TIMEOUT,
            ]
        )
        if result_index == 0:
            print("\tSSH connection refused by host (retry {}/{})".format(retries, max_retries))
            child.kill(0)
            time.sleep(10)
        elif result_index == 1 or result_index == 2:
            if result_index == 1:
                child.sendline("yes")
                child.expect(pexpect_prompt)
            print(f"SSH connection established with EC2 instance!")
            return child
        elif result_index >= 3:
            print("\tSSH connection timed out (retry {}/{})".format(retries, max_retries))
            child.kill(0)
            time.sleep(10)
    print("SSH connection refused by host {} times. Giving up now.".format(max_retries))
    exit()


def wait_until_cmd_done(ssh_proc, pexpect_prompt):
    # On a SSH connection, the prompt is printed two times (because of some obscure reason related to encoding and/or color printing on terminal)
    ssh_proc.expect(pexpect_prompt)
    ssh_proc.expect(pexpect_prompt)


def reboot_instance(
    ssh_process, connection_ssh_cmd, timeout, log_file_handle, pexpect_prompt, retries
):
    ssh_process.sendline(" ")
    wait_until_cmd_done(ssh_process, pexpect_prompt)
    ssh_process.sendline("sudo reboot")
    ssh_process.kill(0)
    time.sleep(5)
    ssh_process = attempt_ssh_connection(
        connection_ssh_cmd, timeout, log_file_handle, pexpect_prompt, retries
    )
    return ssh_process
    print("Reboot complete")


def create_or_start_aws_instance(boto3client, region_name, existing_instance_id, ssh_key_name):
    # Connect to existing instance or create a new one

    # Attempt to start existing instance
    if existing_instance_id != "":
        instance_id = existing_instance_id
        result = start_instance(boto3client, instance_id)
        if result is True:
            # Wait for the instance to be running
            wait_for_instance_to_start_or_stop(boto3client, instance_id, stopping=False)
            return instance_id

    # Define the AWS machine variables

    instance_AMI = instance_AMI_dict[
        region_name
    ]  # The base AWS-provided AMI we build our AMI from: AWS Ubuntu Server 20.04 LTS
    instance_type = "g4dn.2xlarge"  # The type of instance we want to create

    print(
        "Creating AWS EC2 instance of size: {} and with AMI: {}...".format(
            instance_type, instance_AMI
        )
    )

    # Create our EC2 instance
    instance_id = create_ec2_instance(
        boto3client=boto3client,
        region_name=region_name,
        instance_type=instance_type,
        instance_AMI=instance_AMI,
        key_name=ssh_key_name,
        disk_size=64,
    )
    # Give a little time for the instance to be recognized in AWS
    time.sleep(5)

    # Wait for the instance to be running
    wait_for_instance_to_start_or_stop(boto3client, instance_id, stopping=False)

    return instance_id


def clone_whist_repository_on_instance(github_token, pexpect_process, pexpect_prompt):
    # Obtain current branch
    subproc_handle = subprocess.Popen("git branch", shell=True, stdout=subprocess.PIPE)
    subprocess_stdout = subproc_handle.stdout.readlines()

    branch_name = ""
    for line in subprocess_stdout:
        converted_line = line.decode("utf-8").strip()
        if "*" in converted_line:
            branch_name = converted_line[2:]
            break

    print(
        "Cloning branch {} of the fractal/fractal repository on the AWS instance ...".format(
            branch_name
        )
    )

    # Retrieve fractal/fractal monorepo on the instance
    command = (
        "rm -rf fractal; git clone -b "
        + branch_name
        + " https://"
        + github_token
        + "@github.com/fractal/fractal.git | tee ~/github_log.log"
    )
    pexpect_process.sendline(command)
    wait_until_cmd_done(pexpect_process, pexpect_prompt)
    print("Finished downloading fractal/fractal on EC2 instance")


def apply_dpkg_locking_fixup(pexpect_process, pexpect_prompt):
    ## Prevent dpkg locking issues such as the following one:
    # E: Could not get lock /var/lib/dpkg/lock-frontend. It is held by process 2392 (apt-get)
    # E: Unable to acquire the dpkg frontend lock (/var/lib/dpkg/lock-frontend), is another process using it?
    # See also here: https://github.com/ray-project/ray/blob/master/doc/examples/lm/lm-cluster.yaml
    dpkg_commands = [
        "sudo kill -9 `sudo lsof /var/lib/dpkg/lock-frontend | awk '{print $2}' | tail -n 1`",
        "sudo kill -9 `sudo lsof /var/lib/apt/lists/lock | awk '{print $2}' | tail -n 1`",
        "sudo kill -9 `sudo lsof /var/lib/dpkg/lock | awk '{print $2}' | tail -n 1`",
        "sudo killall apt apt-get",
        "sudo pkill -9 apt",
        "sudo pkill -9 apt-get",
        "sudo pkill -9 dpkg",
        "sudo rm /var/lib/apt/lists/lock; sudo rm /var/lib/apt/lists/lock-frontend; sudo rm /var/cache/apt/archives/lock; sudo rm /var/lib/dpkg/lock",
        "sudo dpkg --configure -a",
    ]
    for command in dpkg_commands:
        pexpect_process.sendline(command)
        wait_until_cmd_done(pexpect_process, pexpect_prompt)


def run_host_setup_on_instance(pexpect_process, pexpect_prompt, aws_ssh_cmd, aws_timeout, logfile):
    print("Running the host setup on the instance ...")
    command = "cd ~/fractal/host-setup && ./setup_host.sh --localdevelopment | tee ~/host_setup.log"
    pexpect_process.sendline(command)
    result = pexpect_process.expect([pexpect_prompt, "E: Could not get lock"])
    pexpect_process.expect(pexpect_prompt)

    if result == 1:
        # If still getting lock issues, no alternative but to reboot
        print(
            "Running into severe locking issues (happens frequently), rebooting the instance and trying again!"
        )
        pexpect_process = reboot_instance(
            pexpect_process, aws_ssh_cmd, aws_timeout, logfile, pexpect_prompt, 5
        )
        pexpect_process.sendline(command)
        wait_until_cmd_done(pexpect_process, pexpect_prompt)

    print("Finished running the host setup script on the EC2 instance")
    return pexpect_process


def start_host_service_on_instance(pexpect_process):
    print("Starting the host service on the EC2 instance...")
    command = (
        "sudo rm -rf /fractal && cd ~/fractal/host-service && make run | tee ~/host_service.log"
    )
    pexpect_process.sendline(command)
    pexpect_process.expect("Entering event loop...")
    print("Host service is ready!")


def build_server_on_instance(pexpect_process, pexpect_prompt):
    print("Building the server mandelbox in {} mode ...".format(args.cmake_build_type))
    command = "cd ~/fractal/mandelboxes && ./build.sh browsers/chrome --{} | tee ~/server_mandelbox_build.log".format(
        args.cmake_build_type
    )
    pexpect_process.sendline(command)
    wait_until_cmd_done(pexpect_process, pexpect_prompt)
    print("Finished building the browsers/chrome (server) mandelbox on the EC2 instance")


def run_server_on_instance(pexpect_process):
    command = (
        "cd ~/fractal/mandelboxes && ./run.sh browsers/chrome | tee ~/server_mandelbox_run.log"
    )
    pexpect_process.sendline(command)
    pexpect_process.expect(":/#")
    server_mandelbox_output = pexpect_process.before.decode("utf-8").strip().split("\n")
    server_docker_id = (
        server_mandelbox_output[-2].replace("\n", "").replace("\r", "").replace(" ", "")
    )
    print(
        "Fractal Server started on EC2 instance, on Docker container {}!".format(server_docker_id)
    )

    # Retrieve connection configs from server
    json_data = {}
    for line in server_mandelbox_output:
        if "linux/macos" in line:
            config_vals = line.strip().split()
            server_addr = config_vals[2]
            port_mappings = config_vals[3]
            # Remove leading '-p' chars
            port_mappings = port_mappings[2:].split(".")
            port_32262 = port_mappings[0].split(":")[1]
            port_32263 = port_mappings[1].split(":")[1]
            port_32273 = port_mappings[2].split(":")[1]
            aes_key = config_vals[5]
            json_data["perf_client_server_ip"] = server_addr
            json_data["perf_client_server_port_32262"] = port_32262
            json_data["perf_client_server_port_32263"] = port_32263
            json_data["perf_client_server_port_32273"] = port_32273
            json_data["perf_client_server_aes_key"] = aes_key
            json_data["initial_url"] = testing_url
    return server_docker_id, json_data


def build_client_on_instance(pexpect_process, pexpect_prompt, testing_time):
    # Edit the run-fractal-client.sh to make the client quit after the experiment is over
    print("Setting the experiment duration to {}s...".format(testing_time))
    command = "sed -i 's/timeout 240s/timeout {}s/g' ~/fractal/mandelboxes/development/client/run-fractal-client.sh".format(
        testing_time
    )
    pexpect_process.sendline(command)
    wait_until_cmd_done(pexpect_process, pexpect_prompt)

    print("Building the dev client mandelbox in {} mode ...".format(args.cmake_build_type))
    command = "cd ~/fractal/mandelboxes && ./build.sh development/client --{} | tee ~/client_mandelbox_build.log".format(
        args.cmake_build_type
    )
    pexpect_process.sendline(command)
    wait_until_cmd_done(pexpect_process, pexpect_prompt)
    print("Finished building the dev client mandelbox on the EC2 instance")


def run_client_on_instance(pexpect_process, json_data):
    print("Running the dev client mandelbox, and connecting to the server!")
    command = "cd ~/fractal/mandelboxes && ./run.sh development/client --json-data='{}'".format(
        json.dumps(json_data)
    )
    pexpect_process.sendline(command)
    pexpect_process.expect(":/#")
    client_mandelbox_output = pexpect_process.before.decode("utf-8").strip().split("\n")
    client_docker_id = (
        client_mandelbox_output[-2].replace("\n", "").replace("\r", "").replace(" ", "")
    )
    print(
        "Fractal dev client started on EC2 instance, on Docker container {}!".format(
            client_docker_id
        )
    )
    return client_docker_id


def extract_server_logs_from_instance(
    pexpect_process,
    pexpect_prompt,
    server_docker_id,
    ssh_key_path,
    username,
    server_hostname,
    aws_timeout,
    perf_logs_folder_name,
    log_grabber_log,
):
    command = "rm -rf ~/perf_logs/server; mkdir -p ~/perf_logs/server"
    pexpect_process.sendline(command)
    wait_until_cmd_done(pexpect_process, pexpect_prompt)

    server_logfiles = [
        "/usr/share/fractal/server.log",
        "/usr/share/fractal/teleport.log",
        "/usr/share/fractal/display.log",
    ]
    for server_file_path in server_logfiles:
        command = "docker cp {}:{} ~/perf_logs/server/".format(server_docker_id, server_file_path)
        pexpect_process.sendline(command)
        wait_until_cmd_done(pexpect_process, pexpect_prompt)

    # Download all the logs from the AWS machine
    command = "scp -r -i {} {}@{}:~/perf_logs/server {}".format(
        ssh_key_path, username, server_hostname, perf_logs_folder_name
    )

    local_process = pexpect.spawn(command, timeout=aws_timeout, logfile=log_grabber_log.buffer)
    local_process.expect(["\$", pexpect.EOF])
    local_process.kill(0)


def extract_client_logs_from_instance(
    pexpect_process,
    pexpect_prompt,
    client_docker_id,
    ssh_key_path,
    username,
    client_hostname,
    aws_timeout,
    perf_logs_folder_name,
    log_grabber_log,
):
    command = "rm -rf ~/perf_logs/client; mkdir -p ~/perf_logs/client"
    log_grabber_process.sendline(command)
    wait_until_cmd_done(log_grabber_process, pexpect_prompt)

    client_logfiles = ["/usr/share/fractal/client.log", "/usr/share/fractal/display.log"]
    for client_file_path in client_logfiles:
        command = "docker cp {}:{} ~/perf_logs/client/".format(client_docker_id, client_file_path)
        log_grabber_process.sendline(command)
        wait_until_cmd_done(log_grabber_process, pexpect_prompt)

    # Download all the logs from the AWS machine
    command = "scp -r -i {} {}@{}:~/perf_logs/client {}".format(
        ssh_key_path, username, client_hostname, perf_logs_folder_name
    )

    local_process = pexpect.spawn(command, timeout=aws_timeout, logfile=log_grabber_log.buffer)
    local_process.expect(["\$", pexpect.EOF])
    local_process.kill(0)


def terminate_or_stop_aws_instance(boto3client, instance_id, should_terminate):
    if should_terminate:
        # Terminating the instance and waiting for them to shutdown
        print(f"Testing complete, terminating EC2 instance")
        boto3.client("ec2").terminate_instances(InstanceIds=[instance_id])
    else:
        # Stopping the instance and waiting for it to shutdown
        print(f"Testing complete, stopping EC2 instance")
        result = stop_instance(boto3client, instance_id)
        if result is False:
            printf("Error while stopping the EC2 instance!")
            return

    # Wait for the instance to be terminated
    wait_for_instance_to_start_or_stop(boto3client, instance_id, stopping=True)


# This main loop creates two AWS EC2 instances, one client, one server, and sets up
# a protocol streaming test between them
if __name__ == "__main__":
    # Retrieve args
    ssh_key_name = args.ssh_key_name  # In CI, this is "protocol_performance_testing_sshkey"
    ssh_key_path = (
        args.ssh_key_path
    )  # This is likely /Users/[username]/.ssh/id_rsa if you're on macOS and installed to the system location
    github_token = args.github_token  # The PAT allowing us to fetch code from GitHub

    # Experiment parameters
    testing_url = args.testing_url
    testing_time = args.testing_time

    # Load the SSH key
    if not os.path.isfile(ssh_key_path):
        print("SSH key file {} does not exist".format(ssh_key_path))
        exit()

    # Obtain region name
    region_name = args.region_name

    # Obtain boto3 client
    boto3client = get_boto3client(region_name)

    instance_id = create_or_start_aws_instance(
        boto3client, region_name, args.use_existing_instance, ssh_key_name
    )

    # Get the IP address of the instance
    instance_ip = get_instance_ip(boto3client, instance_id)
    hostname = instance_ip[0]["public"]
    private_ip = instance_ip[0]["private"]
    print("Connected to AWS instance with hostname: {}!".format(hostname))

    host_service_log = open("host_service_logging_file.txt", "w")
    server_log = open("server_log.txt", "w")
    client_log = open("client_log.txt", "w")
    log_grabber_log = open("log_grabber_log.txt", "w")

    username = "ubuntu"
    pexpect_prompt = "{}@ip-{}".format(username, private_ip.replace(".", "-"))
    aws_timeout = 1200  # 10 mins is not enough to build the base mandelbox, so we'll go ahead with 20 mins to be safe

    # Initiate the SSH connections with the instance
    print("Initiating 1° SSH connection with the AWS instance...")
    cmd = "ssh {}@{} -i {}".format(username, hostname, ssh_key_path)
    hs_process = attempt_ssh_connection(cmd, aws_timeout, host_service_log, pexpect_prompt, 5)
    hs_process.expect(pexpect_prompt)

    clone_whist_repository_on_instance(github_token, hs_process, pexpect_prompt)
    apply_dpkg_locking_fixup(hs_process, pexpect_prompt)

    # Set up the server/client
    # 1- run host-setup
    hs_process = run_host_setup_on_instance(
        hs_process, pexpect_prompt, cmd, aws_timeout, host_service_log
    )

    # 2- reboot and wait for it to come back up
    print("Rebooting the EC2 instance (required after running the host setup)...")
    hs_process = reboot_instance(hs_process, cmd, aws_timeout, host_service_log, pexpect_prompt, 5)

    # 3- build and run host-service
    start_host_service_on_instance(hs_process)

    # 4- Build the protocol server
    print("Initiating 2° SSH connection with the AWS instance...")
    server_process = attempt_ssh_connection(cmd, aws_timeout, server_log, pexpect_prompt, 5)
    build_server_on_instance(server_process, pexpect_prompt)

    # 5- Run the protocol server, and retrieve the connection configs
    server_docker_id, json_data = run_server_on_instance(server_process)

    # 6- Build the dev client
    print("Initiating 3° SSH connection with the AWS instance...")
    client_process = attempt_ssh_connection(cmd, aws_timeout, client_log, pexpect_prompt, 5)
    build_client_on_instance(client_process, pexpect_prompt, testing_time)

    # 7- Run the dev client
    client_docker_id = run_client_on_instance(client_process, json_data)

    # Wait <testing_time> seconds to generate enough data
    time.sleep(testing_time)

    # 8- Extract the client/server perf logs from the two docker containers
    print("Initiating 4° SSH connection with the AWS instance...")
    log_grabber_process = attempt_ssh_connection(
        cmd, aws_timeout, log_grabber_log, pexpect_prompt, 5
    )
    log_grabber_process.expect(pexpect_prompt)

    # Create folder for logs
    perf_logs_folder_name = time.strftime("%Y_%m_%d@%H-%M-%S")
    perf_logs_folder_name = "./perf_logs/{}".format(perf_logs_folder_name)
    command = "mkdir -p {}/server".format(perf_logs_folder_name)
    local_process = pexpect.spawn(command, timeout=aws_timeout, logfile=log_grabber_log.buffer)
    local_process.expect(["\$", "%", pexpect.EOF])
    local_process.kill(0)

    command = "mkdir -p {}/client".format(perf_logs_folder_name)
    local_process = pexpect.spawn(command, timeout=aws_timeout, logfile=log_grabber_log.buffer)
    local_process.expect(["\$", "%", pexpect.EOF])
    local_process.kill(0)

    extract_server_logs_from_instance(
        log_grabber_process,
        pexpect_prompt,
        server_docker_id,
        ssh_key_path,
        username,
        hostname,
        aws_timeout,
        perf_logs_folder_name,
        log_grabber_log,
    )
    extract_client_logs_from_instance(
        log_grabber_process,
        pexpect_prompt,
        client_docker_id,
        ssh_key_path,
        username,
        hostname,
        aws_timeout,
        perf_logs_folder_name,
        log_grabber_log,
    )

    # 9 - Clean up the instances
    # Exit the server/client mandelboxes
    server_process.sendline("exit")
    wait_until_cmd_done(server_process, pexpect_prompt)
    client_process.sendline("exit")
    wait_until_cmd_done(client_process, pexpect_prompt)

    # Delete all Docker containers
    command = "docker stop $(docker ps -aq) && docker rm $(docker ps -aq)"
    server_process.sendline(command)
    wait_until_cmd_done(server_process, pexpect_prompt)

    # Terminate the host service
    hs_process.sendcontrol("c")

    # Terminate all pexpect processes
    hs_process.kill(0)
    server_process.kill(0)
    client_process.kill(0)
    log_grabber_process.kill(0)
    local_process.kill(0)

    # Close all log files
    host_service_log.close()
    server_log.close()
    client_log.close()
    log_grabber_log.close()

    # 10 - Terminate or stop AWS instance
    terminate_or_stop_aws_instance(
        boto3client, instance_id, instance_id != args.use_existing_instance
    )
    print(
        "Instance successfully {}, goodbye".format(
            "terminated" if instance_id != args.use_existing_instance else "stopped"
        )
    )

    print("Done")
