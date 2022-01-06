#!/usr/bin/env python3

import pexpect
import json

# Get tools to run operations on a dev instance via SSH
from dev_instance_tools import (
    reboot_instance,
    wait_until_cmd_done,
    attempt_ssh_connection,
    configure_aws_credentials,
    apply_dpkg_locking_fixup,
    clone_whist_repository_on_instance,
)


def run_host_setup_on_instance(pexpect_process, pexpect_prompt, aws_ssh_cmd, aws_timeout, logfile):
    print("Running the host setup on the instance ...")
    command = "cd ~/whist/host-setup && ./setup_host.sh --localdevelopment | tee ~/host_setup.log"
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
        "sudo rm -rf /whist && cd ~/whist/backend/host-service && make run | tee ~/host_service.log"
    )
    pexpect_process.sendline(command)
    pexpect_process.expect("Entering event loop...")
    print("Host service is ready!")


def build_server_on_instance(pexpect_process, pexpect_prompt, cmake_build_type):
    print("Building the server mandelbox in {} mode ...".format(cmake_build_type))
    command = "cd ~/whist/mandelboxes && ./build.sh browsers/chrome --{} | tee ~/server_mandelbox_build.log".format(
        cmake_build_type
    )
    pexpect_process.sendline(command)
    wait_until_cmd_done(pexpect_process, pexpect_prompt)
    print("Finished building the browsers/chrome (server) mandelbox on the EC2 instance")


def run_server_on_instance(pexpect_process):
    command = "cd ~/whist/mandelboxes && ./run.sh browsers/chrome | tee ~/server_mandelbox_run.log"
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


def build_client_on_instance(pexpect_process, pexpect_prompt, testing_time, cmake_build_type):
    # Edit the run-whist-client.sh to make the client quit after the experiment is over
    print("Setting the experiment duration to {}s...".format(testing_time))
    command = "sed -i 's/timeout 240s/timeout {}s/g' ~/whist/mandelboxes/development/client/run-whist-client.sh".format(
        testing_time
    )
    pexpect_process.sendline(command)
    wait_until_cmd_done(pexpect_process, pexpect_prompt)

    print("Building the dev client mandelbox in {} mode ...".format(cmake_build_type))
    command = "cd ~/whist/mandelboxes && ./build.sh development/client --{} | tee ~/client_mandelbox_build.log".format(
        cmake_build_type
    )
    pexpect_process.sendline(command)
    wait_until_cmd_done(pexpect_process, pexpect_prompt)
    print("Finished building the dev client mandelbox on the EC2 instance")


def run_client_on_instance(pexpect_process, json_data):
    print("Running the dev client mandelbox, and connecting to the server!")
    command = "cd ~/whist/mandelboxes && ./run.sh development/client --json-data='{}'".format(
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


def server_setup_process(args_dict):
    username = args_dict["username"]
    server_hostname = args_dict["server_hostname"]
    ssh_key_path = args_dict["ssh_key_path"]
    aws_timeout = args_dict["aws_timeout"]
    server_log_filepath = args_dict["server_log_filepath"]
    pexpect_prompt_server = args_dict["pexpect_prompt_server"]
    github_token = args_dict["github_token"]
    aws_credentials_filepath = args_dict["aws_credentials_filepath"]
    cmake_build_type = args_dict["cmake_build_type"]

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
    build_server_on_instance(hs_process, pexpect_prompt_server, cmake_build_type)

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
    cmake_build_type = args_dict["cmake_build_type"]

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
    build_client_on_instance(
        client_pexpect_process, pexpect_prompt_client, testing_time, cmake_build_type
    )
    client_pexpect_process.kill(0)

    client_log.close()
