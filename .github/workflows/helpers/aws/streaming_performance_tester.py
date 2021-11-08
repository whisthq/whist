#!/usr/bin/env python3

import sys
import os
import time
import boto3
import paramiko

from aws.testing_helpers import (
    create_ec2_instance,
    wait_for_instance_to_start_or_stop,
    get_instance_ip,
    wait_for_ssh,
    run_ssh_command,
)

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


# This main loop creates two AWS EC2 instances, one client, one server, and sets up
# a protocol streaming test between them
if __name__ == "__main__":
    try:
        # Define server machine variables
        ssh_key_name = "protocol_performance_testing_sshkey" # This is configured in AWS EC2 "Key Pairs" section
        instance_AMI = "ami-0885b1f6bd170450c" # The base AWS-provided AMI we build our AMI from: AWS Ubuntu Server 20.04 LTS
        server_instance_type = "g4dn.2xlarge" # The type of instance we want to create
        client_instance_type = "t3.large" # A smaller instance type to run the client on

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

        # Initiate two SSH connections with the two instances
        wait_for_ssh(server_instance_ip)
        print("It works up to here, so far")
        print(f"SSH connection to server instance {server_instance_id} established")

        # Sleep so that the SSH connections resets
        time.sleep(1)

        # At this point, we have both a server and a client running properly.
        # Retrieve fractal/fractal monorepo on each instance
        run_ssh_command(ip=client_instance_ip, cmd="git clone \"https://${{ secrets.GHA_PERSONAL_ACCESS_TOKEN }}@github.com/fractal/fractal.git\" ~/fractal", display_res=False)
        run_ssh_command(ip=server_instance_ip, cmd="git clone \"https://${{ secrets.GHA_PERSONAL_ACCESS_TOKEN }}@github.com/fractal/fractal.git\" ~/fractal", display_res=False)

        # Set up the server first
        # 1- run host-setup
        command = "cd ~/fractal/host-setup && ./setup_host.sh --localdevelopment"
        run_ssh_command(ip=server_instance_ip, cmd=command, display_res=False)

        # 2- reboot and wait for it to come back up
        run_ssh_command(ip=server_instance_ip, cmd="sudo reboot", display_res=False)
        wait_for_ssh(server_instance_ip)

        # 3- build and run host-service
        command = "cd ~/fractal/host-service && make run"
        run_ssh_command(ip=server_instance_ip, cmd=command, display_res=False)

        # 4- Run the protocol server, and retrieve the connection string
        command = "cd ~/fractal/protocol && ./build_protocol_targets.sh --cmakebuildtype=Debug --cmakesetCI FractalServer && ./fserver"
        unparsed_fclient_string = run_ssh_command(ip=server_instance_ip, cmd=command, display_res=True) # True here, we want to receive the ./fclient string
        print(f"Unparsed ./fclient connection string is: {unparsed_fclient_string}")

        # 5- parse the connection string into something readable
        temp = unparsed_fclient_string.split(b'\n')
        client_command = temp[6][temp[6].index(b'.')+2:].decode("utf-8")
        print(f"Parsed ./fclient connection string is: {client_command}")

        # Set up the client
        # 1- Build the protocol client
        command = "cd ~/fractal/protocol && ./build_protocol_targets.sh --cmakebuildtype=Debug --cmakesetCI FractalClient"
        run_ssh_command(ip=client_instance_ip, cmd=command, display_res=False)

        # 2- Run the protocol client with the connection string
        command = "cd ~/fractal/protocol && ./fclient " + client_command 
        run_ssh_command(ip=client_instance_ip, cmd=command, display_res=False)

        # TODO
        # Here we will want to read the .log file and retrieve all of the info we want. We could display it as 
        # a comment on the PR using Neil's Slack bot automator (see webserver DB migration, which does this) or
        # eventually, run this nightly and put it up on Logz.io


    except:  # Don't throw error for keyboard interrupt or timeout
        pass
    finally:
        # Terminating the instances and waiting for them to shutdown
        print(f"Testing complete, terminating client and server instances")
        boto3.client("ec2").terminate_instances(InstanceIds=[client_instance_id])
        boto3.client("ec2").terminate_instances(InstanceIds=[server_instance_id])

        # Wait for the two instances to be terminated
        wait_for_instance_to_start_or_stop(client_instance_id, stopping=True)
        wait_for_instance_to_start_or_stop(server_instance_id, stopping=True)
        print("Instances successfully terminated, goodbye")
        print("Done")
