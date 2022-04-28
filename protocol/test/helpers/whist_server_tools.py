#!/usr/bin/env python3

import os, sys

from helpers.common.ssh_tools import (
    attempt_ssh_connection,
    expression_in_pexpect_output,
    wait_until_cmd_done,
    reboot_instance,
)

from helpers.setup.instance_setup_tools import (
    install_and_configure_aws,
    clone_whist_repository,
    run_host_setup,
    prune_containers_if_needed,
    prepare_instance_for_host_setup,
)

from helpers.common.git_tools import get_remote_whist_github_sha, get_whist_github_sha
from helpers.common.timestamps_and_exit_tools import exit_with_error

# Add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


def server_setup_process(args_dict):
    """
    Prepare a remote Ubuntu machine to host the Whist server. This entails:
    - opening a SSH connection to the machine
    - installing the AWS CLI and setting up the AWS credentials
    - pruning the docker containers if we are running out of space on disk
    - cloning the current branch of the Whist repository if needed
    - launching the Whist host setup if needed
    - building the server mandelbox

    In case of error, this function makes the process running it exit with exitcode -1.

    Args:
        args_dict (multiprocessing.managers.DictProxy): A dictionary containing the configs needed to access the remote
                                                        machine and get a Whist server ready for execution

    Returns:
        None
    """
    username = args_dict["username"]
    server_hostname = args_dict["server_hostname"]
    ssh_key_path = args_dict["ssh_key_path"]
    aws_timeout_seconds = args_dict["aws_timeout_seconds"]
    server_log_filepath = args_dict["server_log_filepath"]
    pexpect_prompt_server = args_dict["pexpect_prompt_server"]
    github_token = args_dict["github_token"]
    aws_credentials_filepath = args_dict["aws_credentials_filepath"]
    cmake_build_type = args_dict["cmake_build_type"]
    running_in_ci = args_dict["running_in_ci"]
    skip_git_clone = args_dict["skip_git_clone"]
    skip_host_setup = args_dict["skip_host_setup"]
    ssh_connection_retries = args_dict["ssh_connection_retries"]

    server_log = open(server_log_filepath, "w")

    # Initiate the SSH connections with the instance
    print("Initiating the SETUP ssh connection with the server AWS instance...")
    server_cmd = f"ssh {username}@{server_hostname} -i {ssh_key_path}"
    hs_process = attempt_ssh_connection(
        server_cmd,
        aws_timeout_seconds,
        server_log,
        pexpect_prompt_server,
        ssh_connection_retries,
        running_in_ci,
    )

    print("Running pre-host-setup on the instance...")
    prepare_instance_for_host_setup(hs_process, pexpect_prompt_server, running_in_ci)

    print("Configuring AWS credentials on server instance...")
    install_and_configure_aws(
        hs_process,
        pexpect_prompt_server,
        running_in_ci,
        aws_credentials_filepath,
    )

    prune_containers_if_needed(hs_process, pexpect_prompt_server, running_in_ci)

    if skip_git_clone == "false":
        clone_whist_repository(github_token, hs_process, pexpect_prompt_server, running_in_ci)
    else:
        print("Skipping git clone whisthq/whist repository on server instance.")

    # Ensure that the commit hash on server matches the one on the runner
    server_sha = get_remote_whist_github_sha(hs_process, pexpect_prompt_server, running_in_ci)
    local_sha = get_whist_github_sha(running_in_ci)
    if server_sha != local_sha:
        exit_with_error(
            f"Commit mismatch between server instance ({server_sha}) and github runner ({local_sha})"
        )

    if skip_host_setup == "false":
        run_host_setup(
            hs_process,
            pexpect_prompt_server,
            running_in_ci,
        )
    else:
        print("Skipping host setup on server instance.")

    # Reboot and wait for it to come back up
    print("Rebooting the server EC2 instance (required after running the host-setup)...")
    hs_process = reboot_instance(
        hs_process,
        server_cmd,
        aws_timeout_seconds,
        server_log,
        pexpect_prompt_server,
        ssh_connection_retries,
        running_in_ci,
    )

    # Build the protocol server
    build_server_on_instance(hs_process, pexpect_prompt_server, cmake_build_type, running_in_ci)

    hs_process.kill(0)
    server_log.close()


def build_server_on_instance(pexpect_process, pexpect_prompt, cmake_build_type, running_in_ci):
    """
    Build the Whist server (browsers/chrome mandelbox) on a remote machine accessible via a SSH
    connection within a pexpect process.

    The function assumes that the pexpect_process process has already successfully established a
    SSH connection to the host, and that the Whist repository has already been cloned.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn):  The Pexpect process created with pexpect.spawn(...) and to be used to
                                                    interact with the remote machine
        pexpect_prompt (str):   The bash prompt printed by the shell on the remote machine when
                                it is ready to execute a command
        cmake_build_type (str): A string identifying whether to build the protocol in release,
                                debug, metrics, or any other Cmake build mode that will be introduced later.
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI

    Returns:
        None
    """
    print(f"Building the server mandelbox in {cmake_build_type} mode ...")
    command = f"cd ~/whist/mandelboxes && ./build.sh browsers/chrome --{cmake_build_type} | tee ~/server_mandelbox_build.log"
    pexpect_process.sendline(command)
    wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)
    print("Finished building the browsers/chrome (server) mandelbox on the EC2 instance")


def run_server_on_instance(pexpect_process):
    """
    Run the Whist server (browsers/chrome mandelbox) on a remote machine accessible via a SSH
    connection within a pexpect process.

    The function assumes that the pexpect_process process has already successfully established
    a SSH connection to the host, that the Whist repository has already been cloned, and that
    the browsers/chrome mandelbox has already been built. Further, the host service must be
    already running on the remote machine.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn):  The Pexpect process created with pexpect.spawn(...) and to be used to
                                                    interact with the remote machine

    Returns:
        server_docker_id (str): The Docker ID of the container running the Whist server
                                (browsers/chrome mandelbox) on the remote machine
        json_data (str):    A dictionary containing the IP, AES KEY, and port mappings that are needed by
                            the client to successfully connect to the Whist server.
    """
    command = "cd ~/whist/mandelboxes && ./run.sh browsers/chrome | tee ~/server_mandelbox_run.log"
    pexpect_process.sendline(command)
    # Need to wait for special mandelbox prompt ":/#". running_in_ci must always be set to True in this case.
    server_mandelbox_output = wait_until_cmd_done(
        pexpect_process, ":/#", running_in_ci=True, return_output=True
    )
    server_docker_id = server_mandelbox_output[-2].replace(" ", "")
    print(f"Whist Server started on EC2 instance, on Docker container {server_docker_id}!")

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


def shutdown_and_wait_server_exit(pexpect_process, exit_confirm_exp):
    """
    Initiate shutdown and wait for server exit to see if the server hangs or exits gracefully

    Args:
        pexpect_process (pexpect.pty_spawn.spawn):  Server pexpect process - MUST BE AFTER DOCKER COMMAND WAS RUN - otherwise
                                                    behavior is undefined
        exit_confirm_exp (str): Target expression to expect on a graceful server exit

    Returns:
        server_has_exited (bool): A boolean set to True if server has exited gracefully, false otherwise
    """
    # Shut down Chrome
    pexpect_process.sendline("pkill chrome")

    # We set running_in_ci=True because the Docker bash does not print in color
    # (check wait_until_cmd_done docstring for more details about handling color bash stdout)
    wait_until_cmd_done(pexpect_process, ":/#", running_in_ci=True)

    # Give WhistServer 20s to shutdown properly
    pexpect_process.sendline("sleep 20")
    wait_until_cmd_done(pexpect_process, ":/#", running_in_ci=True)

    # Check the log to see if WhistServer shut down gracefully or if there was a server hang
    pexpect_process.sendline("tail /var/log/whist/protocol-out.log")
    server_mandelbox_output = wait_until_cmd_done(
        pexpect_process, ":/#", running_in_ci=True, return_output=True
    )
    server_has_exited = expression_in_pexpect_output(exit_confirm_exp, server_mandelbox_output)

    # Kill tail process
    pexpect_process.sendcontrol("c")
    return server_has_exited
