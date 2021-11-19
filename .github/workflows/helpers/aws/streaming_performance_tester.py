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
    create_ec2_instance,
    wait_for_instance_to_start_or_stop,
    get_instance_ip,
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
    "--testing_url",
    help="The URL to visit for testing",
    type=str,
    default="https://fractal-test-assets.s3.amazonaws.com/SpaceX+Launches+4K+Demo.mp4",
)

parser.add_argument(
    "--testing_time",
    help="The length of the perf test in seconds",
    type=int,
    default=126, # Length of the video in link above is 2mins, 6seconds
)

args = parser.parse_args()


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
            print("SSH connection timed out!")
            child.kill(0)
            exit()
    print("SSH connection refused by host {} times. Giving up now.".format(max_retries))
    exit()


def reboot_instance(
    ssh_process, connection_ssh_cmd, timeout, log_file_handle, pexpect_prompt, retries
):
    ssh_process.sendline(" ")
    ssh_process.expect(pexpect_prompt)
    ssh_process.expect(pexpect_prompt)
    ssh_process.sendline("sudo reboot")
    ssh_process.kill(0)
    time.sleep(5)
    ssh_process = attempt_ssh_connection(
        connection_ssh_cmd, timeout, log_file_handle, pexpect_prompt, retries
    )
    return ssh_process
    print("Reboot complete")


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

    # Define the AWS machine variables
    instance_AMI = "ami-0885b1f6bd170450c"  # The base AWS-provided AMI we build our AMI from: AWS Ubuntu Server 20.04 LTS
    instance_type = "g4dn.2xlarge"  # The type of instance we want to create

    print(
        "Creating AWS EC2 instance of size: {} and with AMI: {}...".format(
            instance_type, instance_AMI
        )
    )

    # Create our EC2 instance
    instance_id = create_ec2_instance(
        instance_type=instance_type, instance_AMI=instance_AMI, key_name=ssh_key_name, disk_size=64
    )
    # Give a little time for the instance to be recognized in AWS
    time.sleep(5)

    # Wait for the instance to be running
    wait_for_instance_to_start_or_stop(instance_id, stopping=False)

    # Get the IP address of the instance
    instance_ip = get_instance_ip(instance_id)
    hostname = instance_ip[0]["public"]
    # hostname = "18.206.15.57"
    username = "ubuntu"
    print("Created instance AWS instance with hostname: {}!".format(hostname))

    host_service_log = open("host_service_logging_file.txt", "w")
    server_log = open("server_log.txt", "w")
    client_log = open("client_log.txt", "w")
    log_grabber_log = open("log_grabber_log.txt", "w")
    private_ip = instance_ip[0]["private"]

    pexpect_prompt = "{}@ip-{}".format(username, private_ip.replace(".", "-"))
    aws_timeout = 1200  # 10 mins is not enough to build the base mandelbox, so we'll go ahead with 20 mins to be safe

    # Initiate the SSH connections with the instance
    print("Initiating 1° SSH connection with the AWS instance...")
    # Hang until the connection is established (give up after 20 mins)
    cmd = "ssh {}@{} -i {}".format(username, hostname, ssh_key_path)

    hs_process = attempt_ssh_connection(cmd, aws_timeout, host_service_log, pexpect_prompt, 5)
    hs_process.expect(pexpect_prompt)

    # Obtain current branch
    subprocess = subprocess.Popen("git branch", shell=True, stdout=subprocess.PIPE)
    subprocess_stdout = subprocess.stdout.readlines()

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
        "git clone -b "
        + branch_name
        + " https://"
        + github_token
        + "@github.com/fractal/fractal.git | tee ~/github_log.log"
    )
    hs_process.sendline(command)
    hs_process.expect(pexpect_prompt)
    hs_process.expect(pexpect_prompt)
    print("Finished downloading fractal/fractal on EC2 instance")

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
        hs_process.sendline(command)
        hs_process.expect(pexpect_prompt)
        hs_process.expect(pexpect_prompt)

    # Set up the server/client
    # 1- run host-setup
    print("Running the host setup on the instance ...")
    command = "cd ~/fractal/host-setup && ./setup_host.sh --localdevelopment | tee ~/host_setup.log"
    # hs_process = attempt_host_setup(hs_process, command, cmd, aws_timeout, host_service_log, pexpect_prompt, 5, 5)
    hs_process.sendline(command)
    result = hs_process.expect([pexpect_prompt, "E: Could not get lock"])
    hs_process.expect(pexpect_prompt)

    if result == 1:
        # If still getting lock issues, no alternative but to reboot
        print("Running into severe locking issues, rebooting the instance!")
        hs_process = reboot_instance(
            hs_process, cmd, aws_timeout, host_service_log, pexpect_prompt, 5
        )
        hs_process.sendline(command)
        hs_process.expect(pexpect_prompt)
        hs_process.expect(pexpect_prompt)

    print("Finished running the host setup script on the EC2 instance")

    # 2- reboot and wait for it to come back up
    print("Rebooting the EC2 instance (required after running the host setup)...")
    hs_process = reboot_instance(hs_process, cmd, aws_timeout, host_service_log, pexpect_prompt, 5)

    # 3- build and run host-service
    print("Starting the host service on the EC2 instance...")
    command = (
        "sudo rm -rf /fractal && cd ~/fractal/host-service && make run | tee ~/host_service.log"
    )
    hs_process.sendline(command)
    hs_process.expect("Entering event loop...")
    print("Host service is ready!")

    # 4- Build the protocol server

    # Initiate the SSH connections with the instance
    print("Initiating 2° SSH connection with the AWS instance...")
    # Hang until the connection is established (give up after 10 mins)
    server_process = attempt_ssh_connection(cmd, aws_timeout, server_log, pexpect_prompt, 5)

    print("Building the server mandelbox in PERF mode ...")
    command = "cd ~/fractal/mandelboxes && ./build.sh browsers/chrome --perf | tee ~/server_mandelbox_build.log"
    server_process.sendline(command)
    server_process.expect(pexpect_prompt)
    server_process.expect(pexpect_prompt)
    print("Finished building the browsers/chrome (server) mandelbox on the EC2 instance")

    # 5- Run the protocol server, and retrieve the connection configs
    command = (
        "cd ~/fractal/mandelboxes && ./run.sh browsers/chrome | tee ~/server_mandelbox_run.log"
    )
    server_process.sendline(command)
    server_process.expect(":/#")
    server_mandelbox_output = server_process.before.decode("utf-8").strip().split("\n")
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

    # 6- Build the dev client

    # Initiate the SSH connections with the instance
    print("Initiating 3° SSH connection with the AWS instance...")
    # Hang until the connection is established (give up after 20 mins)
    client_process = attempt_ssh_connection(cmd, aws_timeout, client_log, pexpect_prompt, 5)

    # Edit the run-fractal-client.sh to make the client quit after the experiment is over
    print("Setting the experiment duration to {}s...".format(testing_time))
    command = "sed -i 's/timeout 240s/timeout {}s/g' ~/fractal/mandelboxes/development/client/run-fractal-client.sh".format(
        testing_time
    )
    client_process.sendline(command)
    client_process.expect(pexpect_prompt)
    client_process.expect(pexpect_prompt)

    print("Building the dev client mandelbox in PERF mode ...")
    command = "cd ~/fractal/mandelboxes && ./build.sh development/client --perf | tee ~/client_mandelbox_build.log"
    client_process.sendline(command)
    client_process.expect(pexpect_prompt)
    client_process.expect(pexpect_prompt)
    print("Finished building the dev client mandelbox on the EC2 instance")

    # 7- Run the protocol client
    print("Running the dev client mandelbox, and connecting to the server!")
    command = "cd ~/fractal/mandelboxes && ./run.sh development/client --json-data='{}'".format(
        json.dumps(json_data)
    )
    client_process.sendline(command)
    client_process.expect(":/#")
    client_mandelbox_output = client_process.before.decode("utf-8").strip().split("\n")
    client_docker_id = (
        client_mandelbox_output[-2].replace("\n", "").replace("\r", "").replace(" ", "")
    )
    print(
        "Fractal dev client started on EC2 instance, on Docker container {}!".format(
            client_docker_id
        )
    )

    # Wait 4 minutes to generate enough data
    time.sleep(testing_time)  # 120 seconds = 2 minutes

    # Initiate the SSH connections with the instance
    print("Initiating 4° SSH connection with the AWS instance...")
    # Hang until the connection is established
    log_grabber_process = attempt_ssh_connection(
        cmd, aws_timeout, log_grabber_log, pexpect_prompt, 5
    )
    log_grabber_process.expect(pexpect_prompt)

    # 8- Extract the client/server perf logs from the two docker containers
    command = "rm -rf ~/perf_logs; mkdir -p ~/perf_logs/client mkdir -p ~/perf_logs/server"
    log_grabber_process.sendline(command)
    log_grabber_process.expect(pexpect_prompt)
    log_grabber_process.expect(pexpect_prompt)

    client_logfiles = ["/usr/share/fractal/client.log", "/usr/share/fractal/display.log"]
    for client_file_path in client_logfiles:
        command = "docker cp {}:{} ~/perf_logs/client/".format(client_docker_id, client_file_path)
        log_grabber_process.sendline(command)
        log_grabber_process.expect(pexpect_prompt)
        log_grabber_process.expect(pexpect_prompt)

    server_logfiles = [
        "/usr/share/fractal/server.log",
        "/usr/share/fractal/teleport.log",
        "/usr/share/fractal/display.log",
    ]
    for server_file_path in server_logfiles:
        command = "docker cp {}:{} ~/perf_logs/server/".format(server_docker_id, server_file_path)
        log_grabber_process.sendline(command)
        log_grabber_process.expect(pexpect_prompt)
        log_grabber_process.expect(pexpect_prompt)

    command = "exit"
    log_grabber_process.sendline(command)
    log_grabber_process.expect(["\$", "%"])

    # Copy over all the logs from the AWS machine
    command = "scp -i {} -r {}@{}:~/perf_logs .".format(ssh_key_path, username, hostname)
    local_process = pexpect.spawn(command, timeout=aws_timeout, logfile=log_grabber_log.buffer)
    local_process.expect(["\$", "%"])

    # Exit the server/client mandelboxes
    server_process.sendline("exit")
    server_process.expect(pexpect_prompt)
    server_process.expect(pexpect_prompt)
    client_process.sendline("exit")
    client_process.expect(pexpect_prompt)
    client_process.expect(pexpect_prompt)

    # Delete all Docker containers
    command = "docker stop $(docker ps -aq) && docker rm $(docker ps -aq)"
    server_process.sendline(command)
    server_process.expect(pexpect_prompt)
    server_process.expect(pexpect_prompt)

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

    # Terminating the instance and waiting for them to shutdown
    print(f"Testing complete, terminating EC2 instance")
    boto3.client("ec2").terminate_instances(InstanceIds=[instance_id])

    # Wait for the instance to be terminated
    wait_for_instance_to_start_or_stop(instance_id, stopping=True)

    print("Instance successfully terminated, goodbye")
    print("Done")
