#!/usr/bin/env python3

import sys
import os
import time
import argparse
import boto3
import paramiko

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
    ssh_key = paramiko.RSAKey.from_private_key_file(ssh_key_path)

    # Define client and server server machine variables
    instance_AMI = "ami-0885b1f6bd170450c"  # The base AWS-provided AMI we build our AMI from: AWS Ubuntu Server 20.04 LTS
    server_instance_type = "g4dn.2xlarge"  # The type of instance we want to create
    client_instance_type = "t3.large"  # A smaller instance type to run the client on

    # Create our two EC2 instances
    client_instance_id = create_ec2_instance(
        instance_type=client_instance_type, instance_AMI=instance_AMI, key_name=ssh_key_name
    )
    server_instance_id = create_ec2_instance(
        instance_type=server_instance_type, instance_AMI=instance_AMI, key_name=ssh_key_name
    )

    # Give a little time for the instance to be recognized in AWS
    time.sleep(5)

    # Wait for the two instances to be running
    wait_for_instance_to_start_or_stop(client_instance_id, stopping=False)
    wait_for_instance_to_start_or_stop(server_instance_id, stopping=False)

    # Get the IP address of the two instances
    client_instance_ip = get_instance_ip(client_instance_id)
    server_instance_ip = get_instance_ip(server_instance_id)

    # # Initiate two SSH connections with the two instances
    # First, the client machine
    client_ssh_client = paramiko.client.SSHClient()
    client_ssh_client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    # Hang until the connection is established
    available = False
    while not available:
        available = True
        try:
            client_ssh_client.connect(
                hostname=client_instance_ip[0]["public"], username="ubuntu", pkey=ssh_key
            )
        except:
            available = False
        finally:
            continue
    print(f"SSH is available on the client EC2 instance: {client_instance_ip[0]['public']}")

    # Now, the server machine
    server_ssh_client = paramiko.client.SSHClient()
    server_ssh_client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    # Hang until the connection is established
    available = False
    while not available:
        available = True
        try:
            server_ssh_client.connect(
                hostname=server_instance_ip[0]["public"], username="ubuntu", pkey=ssh_key
            )
        except:
            available = False
        finally:
            continue
    print(f"SSH is available on the client EC2 instance: {server_instance_ip[0]['public']}")
    print(f"SSH connections established")

    # At this point, we have both a server and a client running properly.
    # Retrieve fractal/fractal monorepo on each instance
    command="git clone https://" + github_token + "@github.com/fractal/fractal.git ~/fractal"
    client_ssh_client.exec_command(command=command)
    stdin, stdout, stderr = server_ssh_client.exec_command(command=command)

    exit_status = stdout.channel.recv_exit_status()          # Blocking call
    if exit_status == 0:
        print ("Finished downloading fractal/fractal on server instance")
    else:
        print("Error", exit_status)

    # Set up the server first
    # 1- run host-setup
    command = "cd ~/fractal/host-setup && ./setup_host.sh --localdevelopment"
    server_ssh_client.exec_command(command=command)

    # 2- reboot and wait for it to come back up
    server_ssh_client.exec_command(command="sudo reboot")
    
    # TODO wait for SSH here 

    # 3- Start the X server
    sftp = paramiko.SFTPClient.from_transport(server_ssh_client)
    sftp.put("./dummy-1920x1080.conf", "dummy-1920x1080.conf")
    sftp.close()
    command = "sudo apt install xserver-xorg-video-dummy && sudo X -config dummy-1920x1080.conf"
    server_ssh_client.exec_command(command=command)


    # 4- build and run host-service
    command = "cd ~/fractal/host-service && make run"
    server_ssh_client.exec_command(command=command)

    # 5- Run the protocol server, and retrieve the connection string
    command = "cd ~/fractal/protocol && ./build_protocol_targets.sh --cmakebuildtype=Debug --cmakesetCI FractalServer && ./fserver"
    _, stdout, _ = server_ssh_client.exec_command(command=command)
    unparsed_fclient_string = ""
    for line in iter(stdout.readline, ""):
        print(line, end="")
        if "linux/macos" in line:
            unparsed_fclient_string = line[line.find(".") + 2 :].strip()
            print(f"Unparsed ./fclient connection string is: {unparsed_fclient_string}")

    # 6- parse the connection string into something readable
    temp = unparsed_fclient_string.split(b"\n")
    client_command = temp[6][temp[6].index(b".") + 2 :].decode("utf-8")
    print(f"Parsed ./fclient connection string is: {client_command}")

    # Set up the client
    # 1- Build the protocol client
    command = "cd ~/fractal/protocol && ./build_protocol_targets.sh --cmakebuildtype=Debug --cmakesetCI FractalClient"
    client_ssh_client.exec_command(command=command)

    # 2- Run the protocol client with the connection string
    command = "cd ~/fractal/protocol && DISPLAY=:0 ./fclient " + client_command
    server_ssh_client.exec_command(command=command)

    # Wait 4 minutes to generate enough data
    time.sleep(240)  # 240 seconds = 4 minutes

    # Close SSH connections
    client_ssh_client.close()
    server_ssh_client.close()

    # TODO
    # Here we will want to read the .log file and retrieve all of the info we want. We could display it as
    # a comment on the PR using Neil's Slack bot automator (see webserver DB migration, which does this) or
    # eventually, run this nightly and put it up on Logz.io

    # Terminating the instances and waiting for them to shutdown
    print(f"Testing complete, terminating client and server instances")
    boto3.client("ec2").terminate_instances(InstanceIds=[client_instance_id])
    boto3.client("ec2").terminate_instances(InstanceIds=[server_instance_id])

    # Wait for the two instances to be terminated
    wait_for_instance_to_start_or_stop(client_instance_id, stopping=True)
    wait_for_instance_to_start_or_stop(server_instance_id, stopping=True)
    print("Instances successfully terminated, goodbye")
    print("Done")
