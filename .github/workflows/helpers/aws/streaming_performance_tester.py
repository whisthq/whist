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
import multiprocessing

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

# Ubuntu Server 20 LTS - AMIs for the various AWS regions
instance_AMI_dict = {}
instance_AMI_dict["us-east-1"] = "ami-0885b1f6bd170450c"
instance_AMI_dict["us-east-2"] = "ami-0629230e074c580f2"
instance_AMI_dict["us-west-1"] = "ami-053ac55bdcfe96e85"
instance_AMI_dict["us-west-2"] = "ami-036d46416a34a611c"
instance_AMI_dict["af-south-1"] = "ami-0ff86122fd4ad7208"
instance_AMI_dict["ap-east-1"] = "ami-0a9c1cc3697104990"
instance_AMI_dict["ap-south-1"] = "ami-0108d6a82a783b352"
instance_AMI_dict["ap-northeast-3"] = "ami-0c3904e7363bbc4bc"
instance_AMI_dict["ap-northeast-2"] = "ami-0f8b8babb98cc66d0"
instance_AMI_dict["ap-southeast-1"] = "ami-0fed77069cd5a6d6c"
instance_AMI_dict["ap-southeast-2"] = "ami-0bf8b986de7e3c7ce"
instance_AMI_dict["ap-northeast-1"] = "ami-036d0684fc96830ca"
instance_AMI_dict["ca-central-1"] = "ami-0bb84e7329f4fa1f7"
instance_AMI_dict["eu-central-1"] = "ami-0a49b025fffbbdac6"
instance_AMI_dict["eu-west-1"] = "ami-08edbb0e85d6a0a07"
instance_AMI_dict["eu-west-2"] = "ami-0fdf70ed5c34c5f52"
instance_AMI_dict["eu-south-1"] = "ami-0f8ce9c417115413d"
instance_AMI_dict["eu-west-3"] = "ami-06d79c60d7454e2af"
instance_AMI_dict["eu-north-1"] = "ami-0bd9c26722573e69b"
instance_AMI_dict["sa-east-1"] = "ami-0e66f5495b4efdd0f"


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
    print("Whist Server started on EC2 instance, on Docker container {}!".format(server_docker_id))

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
            json_data["dev_client_server_ip"] = server_addr
            json_data["dev_client_server_port_32262"] = port_32262
            json_data["dev_client_server_port_32263"] = port_32263
            json_data["dev_client_server_port_32273"] = port_32273
            json_data["dev_client_server_aes_key"] = aes_key
    return server_docker_id, json_data


def build_client_on_instance(pexpect_process, pexpect_prompt, testing_time):
    # Edit the run-whist-client.sh to make the client quit after the experiment is over
    print("Setting the experiment duration to {}s...".format(testing_time))
    command = "sed -i 's/timeout 240s/timeout {}s/g' ~/fractal/mandelboxes/development/client/run-whist-client.sh".format(
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
        "Whist dev client started on EC2 instance, on Docker container {}!".format(client_docker_id)
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
        "/usr/share/whist/server.log",
        "/usr/share/whist/teleport.log",
        "/usr/share/whist/display.log",
        # Var Logs!
        "/var/log/whist/audio-err.log",
        "/var/log/whist/audio-out.log",
        "/var/log/whist/display-err.log",
        "/var/log/whist/display-out.log",
        "/var/log/whist/entry-err.log",
        "/var/log/whist/entry-out.log",
        "/var/log/whist/update_xorg_conf-err.log",
        "/var/log/whist/update_xorg_conf-out.log",
        "/var/log/whist/protocol-err.log",
        "/var/log/whist/protocol-out.log",
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
    pexpect_process.sendline(command)
    wait_until_cmd_done(pexpect_process, pexpect_prompt)

    client_logfiles = [
        "/usr/share/whist/client.log",
        "/usr/share/whist/display.log",
        # Var Logs!
        "/var/log/whist/audio-err.log",
        "/var/log/whist/audio-out.log",
        "/var/log/whist/display-err.log",
        "/var/log/whist/display-out.log",
        "/var/log/whist/entry-err.log",
        "/var/log/whist/entry-out.log",
        "/var/log/whist/update_xorg_conf-err.log",
        "/var/log/whist/update_xorg_conf-out.log",
        "/var/log/whist/protocol-err.log",
        "/var/log/whist/protocol-out.log",
    ]
    for client_file_path in client_logfiles:
        command = "docker cp {}:{} ~/perf_logs/client/".format(client_docker_id, client_file_path)
        pexpect_process.sendline(command)
        wait_until_cmd_done(pexpect_process, pexpect_prompt)

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
        boto3client.terminate_instances(InstanceIds=[instance_id])
    else:
        # Stopping the instance and waiting for it to shutdown
        print(f"Testing complete, stopping EC2 instance")
        result = stop_instance(boto3client, instance_id)
        if result is False:
            printf("Error while stopping the EC2 instance!")
            return

    # Wait for the instance to be terminated
    wait_for_instance_to_start_or_stop(boto3client, instance_id, stopping=True)


def configure_aws_credentials(
    pexpect_process, pexpect_prompt, aws_credentials_filepath="~/.aws/credentials"
):

    aws_credentials_filepath_expanded = os.path.expanduser(aws_credentials_filepath)

    if not os.path.isfile(aws_credentials_filepath_expanded):
        print(
            "Could not find local AWS credential file at path {}!".format(aws_credentials_filepath)
        )
        return

    aws_credentials_file = open(aws_credentials_filepath_expanded, "r")
    aws_access_key_id = ""
    aws_secret_access_key = ""
    for line in aws_credentials_file.readlines():
        if "aws_access_key_id" in line:
            aws_access_key_id = line.strip().split()[2]
        elif "aws_secret_access_key" in line:
            aws_secret_access_key = line.strip().split()[2]
            break
    if aws_access_key_id == "" or aws_secret_access_key == "":
        print(
            "Could not parse AWS credentials from file at path {}!".format(aws_credentials_filepath)
        )
        return
    pexpect_process.sendline("sudo apt-get update")
    result = pexpect_process.expect(["Do you want to continue? [Y/n]", pexpect_prompt])
    if result == 0:
        pexpect_process.sendline("Y")
        wait_until_cmd_done(pexpect_process, pexpect_prompt)
    else:
        pexpect_process.expect(pexpect_prompt)
    pexpect_process.sendline("sudo apt-get install awscli")
    wait_until_cmd_done(pexpect_process, pexpect_prompt)
    pexpect_process.sendline("aws configure")
    pexpect_process.expect("AWS Access Key ID")
    pexpect_process.sendline(aws_access_key_id)
    pexpect_process.expect("AWS Secret Access Key")
    pexpect_process.sendline(aws_secret_access_key)
    pexpect_process.expect("Default region name")
    pexpect_process.sendline("")
    pexpect_process.expect("Default output format")
    pexpect_process.sendline("")
    wait_until_cmd_done(pexpect_process, pexpect_prompt)
    aws_credentials_file.close()


def server_setup_process(args_dict):
    username = args_dict["username"]
    server_hostname = args_dict["server_hostname"]
    ssh_key_path = args_dict["ssh_key_path"]
    aws_timeout = args_dict["aws_timeout"]
    server_log_filepath = args_dict["server_log_filepath"]
    pexpect_prompt_server = args_dict["pexpect_prompt_server"]
    github_token = args_dict["github_token"]
    aws_credentials_filepath = args_dict["aws_credentials_filepath"]

    server_log = open(server_log_filepath, "w")

    # Initiate the SSH connections with the instance
    print("Initiating the SETUP ssh connection with the server AWS instance...")
    server_cmd = "ssh {}@{} -i {}".format(username, server_hostname, ssh_key_path)
    hs_process = attempt_ssh_connection(
        server_cmd, aws_timeout, server_log, pexpect_prompt_server, 5
    )
    hs_process.expect(pexpect_prompt_server)

    print("Configuring AWS credentials on server instance...")
    configure_aws_credentials(hs_process, pexpect_prompt_server, aws_credentials_filepath)

    clone_whist_repository_on_instance(github_token, hs_process, pexpect_prompt_server)
    apply_dpkg_locking_fixup(hs_process, pexpect_prompt_server)

    # 1- run host-setup
    hs_process = run_host_setup_on_instance(
        hs_process, pexpect_prompt_server, server_cmd, aws_timeout, server_log
    )

    # 2- reboot and wait for it to come back up
    print("Rebooting the server EC2 instance (required after running the host setup)...")
    hs_process = reboot_instance(
        hs_process, server_cmd, aws_timeout, server_log, pexpect_prompt_server, 5
    )

    # 3- Build the protocol server
    build_server_on_instance(hs_process, pexpect_prompt_server)

    hs_process.kill(0)

    server_log.close()


def client_setup_process(args_dict):
    username = args_dict["username"]
    client_hostname = args_dict["client_hostname"]
    ssh_key_path = args_dict["ssh_key_path"]
    aws_timeout = args_dict["aws_timeout"]
    client_log_filepath = args_dict["client_log_filepath"]
    pexpect_prompt_client = args_dict["pexpect_prompt_client"]
    github_token = args_dict["github_token"]
    use_two_instances = args_dict["use_two_instances"]
    testing_time = args_dict["testing_time"]
    aws_credentials_filepath = args_dict["aws_credentials_filepath"]

    client_log = open(client_log_filepath, "w")

    client_cmd = "ssh {}@{} -i {}".format(username, client_hostname, ssh_key_path)

    if use_two_instances:
        # Initiate the SSH connections with the client instance
        print("Initiating the SETUP ssh connection with the client AWS instance...")
        hs_process = attempt_ssh_connection(
            client_cmd, aws_timeout, client_log, pexpect_prompt_client, 5
        )
        hs_process.expect(pexpect_prompt_client)
        print("Configuring AWS credentials on client instance...")
        configure_aws_credentials(hs_process, pexpect_prompt_client, aws_credentials_filepath)

        clone_whist_repository_on_instance(github_token, hs_process, pexpect_prompt_client)
        apply_dpkg_locking_fixup(hs_process, pexpect_prompt_client)

        # 1- run host-setup
        hs_process = run_host_setup_on_instance(
            hs_process, pexpect_prompt_client, client_cmd, aws_timeout, client_log
        )

        # 2- reboot and wait for it to come back up
        print("Rebooting the server EC2 instance (required after running the host setup)...")
        hs_process = reboot_instance(
            hs_process, client_cmd, aws_timeout, client_log, pexpect_prompt_client, 5
        )

        hs_process.kill(0)

    # 6- Build the dev client
    print("Initiating the BUILD ssh connection with the client AWS instance...")
    client_pexpect_process = attempt_ssh_connection(
        client_cmd, aws_timeout, client_log, pexpect_prompt_client, 5
    )
    build_client_on_instance(client_pexpect_process, pexpect_prompt_client, testing_time)
    client_pexpect_process.kill(0)

    client_log.close()


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
