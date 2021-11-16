#!/usr/bin/env python3

import sys
import os
import time
import argparse
import boto3
import paramiko
import subprocess
import pexpect

from testing_helpers import (
    create_ec2_instance,
    wait_for_instance_to_start_or_stop,
    get_instance_ip,
)

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))

DESCRIPTION = """
This script will spin up 2 EC2 instances (one g4dn.2xlarge server, one t3.large client),
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

args = parser.parse_args()


def attempt_connecting(ssh_command, timeout, log_file_handle, pexpect_prompt, max_retries):
    for retries in range(max_retries):
        child = pexpect.spawn(ssh_command, timeout=timeout, logfile=log_file_handle.buffer)
        result_index = child.expect (['Connection refused', 'Are you sure you want to continue connecting (yes/no/[fingerprint])?', pexpect_prompt, pexpect.EOF, pexpect.TIMEOUT])
        if result_index == 0:
            print("\tSSH connection refused by host (retry {}/{})".format(retries, max_retries))
            child.kill(0)
            time.sleep(10)
        elif result_index == 1 or result_index == 2:
            if result_index == 1:
                child.sendline("yes")
                child.expect(pexpect_prompt)
            print(f"SSH connection established with EC2 instance: {instance_ip[0]['public']}")
            return child
        elif result_index >= 3:
            print("SSH connection timed out!")
            child.kill(0)
            exit()
    print("SSH connection refused by host {} times. Giving up now.".format(max_retries))
    exit()

# This main loop creates two AWS EC2 instances, one client, one server, and sets up
# a protocol streaming test between them
if __name__ == "__main__":
    # Retrieve args
    ssh_key_name = args.ssh_key_name  # In CI, this is "protocol_performance_testing_sshkey"
    ssh_key_path = (
        args.ssh_key_path
    )  # This is likely /Users/[username]/.ssh/id_rsa if you're on macOS and installed to the system location
    github_token = args.github_token # The PAT allowing us to fetch code from GitHub

    # Load the SSH key
    if not os.path.isfile(ssh_key_path):
        print("SSH key file {} does not exist".format(ssh_key_path))
        exit()

    # Define the AWS machine variables
    instance_AMI = "ami-0885b1f6bd170450c"  # The base AWS-provided AMI we build our AMI from: AWS Ubuntu Server 20.04 LTS
    instance_type = "g4dn.2xlarge"  # The type of instance we want to create
    
    print("Creating AWS EC2 instance of size: {} and with AMI: {}...".format(instance_type, instance_AMI))

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
    username = "ubuntu"
    print("Created instance AWS instance with hostname: {}!".format(hostname))
    
    host_service_log = open("host_service_logging_file.txt", "w")
    server_log = open("server_log.txt", "w")
    client_log = open("client_log.txt", "w")
    private_ip = instance_ip[0]["private"]
    pexpect_prompt = "{}@ip-{}:".format(username, private_ip.replace('.','-'))


    # Initiate the SSH connections with the instance
    print("Initiating 1° SSH connection with the AWS instance...")
    # Hang until the connection is established (give up after 10 mins)
    cmd = "ssh {}@{} -i {}".format(username, hostname, ssh_key_path)
    
    hs_process = attempt_connecting(cmd, 600, host_service_log, pexpect_prompt, 5)

    # Obtain current branch
    subprocess = subprocess.Popen("git branch", shell=True, stdout=subprocess.PIPE)
    subprocess_stdout = subprocess.stdout.readlines()

    branch_name=""
    for line in subprocess_stdout:
        converted_line = line.decode('utf-8').strip()
        if "*" in converted_line:
            branch_name = converted_line[2:]
            break

    print("Cloning branch {} of the fractal/fractal repository on the AWS instance ...".format(branch_name))

    # Retrieve fractal/fractal monorepo on the instance
    command="git clone -b " + branch_name + " https://" + github_token + "@github.com/fractal/fractal.git | tee ~/github_log.log"
    hs_process.sendline(command)
    hs_process.expect(pexpect_prompt)
    print ("Finished downloading fractal/fractal on EC2 instance")

    # Set up the server/client
    # 1- run host-setup
    print ("Running the host setup on the instance ...")
    command = "cd ~/fractal/host-setup && ./setup_host.sh --localdevelopment | tee ~/host_setup.log"
    hs_process.sendline(command)
    hs_process.expect(pexpect_prompt)
    print ("Finished running the host setup script on the EC2 instance")

    # TODO: handle occasional host-setup error by waiting 10s, retrying, if still not working, reboot and repeat.
    # E: Could not get lock /var/lib/dpkg/lock-frontend. It is held by process 2392 (apt-get)
    # E: Unable to acquire the dpkg frontend lock (/var/lib/dpkg/lock-frontend), is another process using it?

    # 2- reboot and wait for it to come back up
    print("Rebooting the EC2 instance (required after running the host setup)...")
    hs_process.sendline("sudo reboot")
    hs_process.kill(0)
    time.sleep(5)
    hs_process = attempt_connecting(cmd, 600, host_service_log, pexpect_prompt, 5)
    print("Reboot complete")


    # 3- build and run host-service
    print ("Starting the host service on the EC2 instance...")
    command = "cd ~/fractal/host-service && make run | tee ~/host_service.log"
    hs_process.sendline(command)
    hs_process.expect("Entering event loop...")
    print("Host service is ready!")

    # 4- Build the protocol server
    
    # Initiate the SSH connections with the instance
    print("Initiating 2° SSH connection with the AWS instance...")
    # Hang until the connection is established (give up after 10 mins)
    server_process = attempt_connecting(cmd, 600, server_log, pexpect_prompt, 5)
    
    print("Building the server mandelbox in PERF mode ...")
    command="cd ~/fractal/mandelboxes && ./build.sh browsers/chrome --perf | tee ~/server_mandelbox_build.log"
    server_process.sendline(command)
    server_process.expect(pexpect_prompt)
    print ("Finished building the browsers/chrome (server) mandelbox on the EC2 instance")
    

    # 5- Run the protocol server, and retrieve the connection configs
    command = "cd ~/fractal/mandelboxes && ./run.sh browsers/chrome | tee ~/server_mandelbox_run.log"
    server_process.sendline(command)
    server_process.expect("")
    server_docker_id=stdout.readlines()[-1].strip()
    print("Fractal Server started on EC2 instance, on Docker container {}!".format(server_docker_id))

    
    child.kill(0)
    logging_file.close()

    

    
    
    

    # # Retrieve connection configs from server
    # json_data = {}
    # for line in iter(stdout.readline, ""):
    #     print(line, end="")
    #     if "linux/macos" in line:
    #         config_vals = line.strip().split()
    #         server_addr = config_vals[2]
    #         port_mappings = config_vals[3]
    #         #Remove leading '-p' chars
    #         port_mappings = port_mappings[2:].split(".")
    #         port_32262 = port_mappings[0].split(":")[1]
    #         port_32263 = port_mappings[1].split(":")[1]
    #         port_32273 = port_mappings[2].split(":")[1]
    #         aes_key = config_vals[5]
    #         json_data["perf_client_server_ip"] = server_addr
    #         json_data["perf_client_server_port_32262"] = port_32262
    #         json_data["perf_client_server_port_32263"] = port_32263
    #         json_data["perf_client_server_port_32273"] = port_32273
    #         json_data["perf_client_server_aes_key"] = aes_key

    # # 6- Build the dev client
    # command="cd ~/fractal/mandelboxes && ./build.sh development/client --perf"
    # stdin, stdout, stderr = ssh_client.exec_command(command=command)
    # exit_status = stdout.channel.recv_exit_status()          # Blocking call
    # if exit_status == 0:
    #     print ("Finished building the fractal dev client container")
    # else:
    #     print("Error", exit_status)

    # # 7- Run the protocol client
    # command="cd ~/fractal/mandelboxes && ./run.sh development/client --json-data='{}'".format(json.dumps(json_data))
    # ssh_client.exec_command(command=command)
    # stdin, stdout, stderr = ssh_client.exec_command(command=command)
    # time.sleep(5)
    # client_docker_id=stdout.readlines()[-1].strip()
    # print("Fractal client started on EC2 instance, on Docker container {}!".format(client_docker_id))
    
    # # Wait 4 minutes to generate enough data
    # time.sleep(240)  # 240 seconds = 4 minutes
    
    # # 8- Extract the client/server perf logs from the two docker containers
    # command="mkdir ~/perf_logs && docker cp {}:/usr/share/fractal/client.log ~/perf_logs".format(client_docker_id)
    # ssh_client.exec_command(command=command)
    # command="docker cp {}:/usr/share/fractal/*.log ~/perf_logs".format(server_docker_id)
    # ssh_client.exec_command(command=command)

    # # TODO
    # # Here we will want to read the .logd file and retrieve all of the info we want. We could display it as
    # # a comment on the PR using Neil's Slack bot automator (see webserver DB migration, which does this) or
    # # eventually, run this nightly and put it up on Logz.io
    
    # # Close SSH connection. No need to clean up the instance (e.g. stopping the host service, stopping/removing the docker containers) because we will terminate the instance.
    # ssh_client.close()

    # # Terminating the instance and waiting for them to shutdown
    # print(f"Testing complete, terminating EC2 instance")
    # boto3.client("ec2").terminate_instances(InstanceIds=[instance_id])

    # # Wait for the instance to be terminated
    # wait_for_instance_to_start_or_stop(instance_id, stopping=True)
    # print("Instance successfully terminated, goodbye")
    # print("Done")
