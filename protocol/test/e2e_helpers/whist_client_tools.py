#!/usr/bin/env python3

import os, sys, json, time

from e2e_helpers.common.ssh_tools import (
    attempt_ssh_connection,
    wait_until_cmd_done,
    reboot_instance,
    apply_dpkg_locking_fixup,
)

from e2e_helpers.setup.instance_setup_tools import (
    install_and_configure_aws,
    clone_whist_repository,
    run_host_setup,
    prune_containers_if_needed,
)

from e2e_helpers.setup.network_tools import (
    restore_network_conditions,
)

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


def client_setup_process(args_dict):
    """
    Prepare a remote Ubuntu machine to host the Whist dev client. This entails:
    - opening a SSH connection to the machine
    - if the client is not running on the same machine as the server:
        - restoring the default network conditions, if some artificial network
          degradation was previously applied on the instance
        - installing the AWS CLI and setting up the AWS credentials
        - pruning the docker containers if we are running out of space on disk
        - cloning the current branch of the Whist repository if needed
        - launching the Whist host setup if needed
    - building the client mandelbox

    In case of error, this function makes the process running it exit with exitcode -1.

    Args:
        args_dict: A dictionary containing the configs needed to access the remote machine
                and get a Whist dev client ready for execution

    Returns:
        None
    """
    username = args_dict["username"]
    client_hostname = args_dict["client_hostname"]
    ssh_key_path = args_dict["ssh_key_path"]
    aws_timeout_seconds = args_dict["aws_timeout_seconds"]
    client_log_filepath = args_dict["client_log_filepath"]
    pexpect_prompt_client = args_dict["pexpect_prompt_client"]
    github_token = args_dict["github_token"]
    use_two_instances = args_dict["use_two_instances"]
    testing_time = args_dict["testing_time"]
    aws_credentials_filepath = args_dict["aws_credentials_filepath"]
    cmake_build_type = args_dict["cmake_build_type"]
    running_in_ci = args_dict["running_in_ci"]
    skip_git_clone = args_dict["skip_git_clone"]
    skip_host_setup = args_dict["skip_host_setup"]
    ssh_connection_retries = args_dict["ssh_connection_retries"]

    client_log = open(client_log_filepath, "w")

    client_cmd = f"ssh {username}@{client_hostname} -i {ssh_key_path}"

    # If we are using the same instance for client and server, all the operations in this if-statement have already been done by server_setup_process
    if use_two_instances:
        # Initiate the SSH connections with the client instance
        print("Initiating the SETUP ssh connection with the client AWS instance...")
        hs_process = attempt_ssh_connection(
            client_cmd,
            aws_timeout_seconds,
            client_log,
            pexpect_prompt_client,
            ssh_connection_retries,
            running_in_ci,
        )

        # Restore network conditions in case a previous run failed / was canceled before restoring the normal conditions.
        restore_network_conditions(hs_process, pexpect_prompt_client, running_in_ci)

        print("Configuring AWS credentials on client instance...")
        result = install_and_configure_aws(
            hs_process,
            pexpect_prompt_client,
            aws_timeout_seconds,
            running_in_ci,
            aws_credentials_filepath,
        )
        if not result:
            sys.exit(-1)

        prune_containers_if_needed(hs_process, pexpect_prompt_client, running_in_ci)

        if skip_git_clone == "false":
            clone_whist_repository(github_token, hs_process, pexpect_prompt_client, running_in_ci)
        else:
            print("Skipping git clone whisthq/whist repository on client instance.")

        if skip_host_setup == "false":
            # 1- Reboot instance for extra robustness
            hs_process = reboot_instance(
                hs_process,
                client_cmd,
                aws_timeout_seconds,
                client_log,
                pexpect_prompt_client,
                ssh_connection_retries,
                running_in_ci,
            )

            # 2 - Fix DPKG issue in case it comes up
            apply_dpkg_locking_fixup(hs_process, pexpect_prompt_client, running_in_ci)

            # 3- run host-setup
            hs_process = run_host_setup(
                hs_process,
                pexpect_prompt_client,
                client_cmd,
                ssh_connection_retries,
                aws_timeout_seconds,
                client_log,
                running_in_ci,
            )
        else:
            print("Skipping host setup on server instance.")

        # 2- reboot and wait for it to come back up
        print("Rebooting the client EC2 instance (required after running the host setup)...")
        hs_process = reboot_instance(
            hs_process,
            client_cmd,
            aws_timeout_seconds,
            client_log,
            pexpect_prompt_client,
            ssh_connection_retries,
            running_in_ci,
        )

        hs_process.kill(0)

    # 6- Build the dev client
    print("Initiating the BUILD ssh connection with the client AWS instance...")
    client_pexpect_process = attempt_ssh_connection(
        client_cmd,
        aws_timeout_seconds,
        client_log,
        pexpect_prompt_client,
        ssh_connection_retries,
        running_in_ci,
    )
    build_client_on_instance(
        client_pexpect_process, pexpect_prompt_client, testing_time, cmake_build_type, running_in_ci
    )
    client_pexpect_process.kill(0)

    client_log.close()


def build_client_on_instance(
    pexpect_process, pexpect_prompt, testing_time, cmake_build_type, running_in_ci
):
    """
    Build the Whist protocol client (development/client mandelbox) on a remote machine accessible
    via a SSH connection within a pexpect process.

    The function assumes that the pexpect_process process has already successfully established a
    SSH connection to the host, and that the Whist repository has already been cloned.

    Args:
        pexpect_process: The Pexpect process created with pexpect.spawn(...) and to be used to interact
                        with the remote machine
        pexpect_prompt: The bash prompt printed by the shell on the remote machine when it is ready to
                        execute a command
        testing_time: The amount of time to leave the connection open between the client and the server
                        (when the client is started) before shutting it down
        cmake_build_type: A string identifying whether to build the protocol in release, debug, metrics,
                        or any other Cmake build mode that will be introduced later.
        running_in_ci: A boolean indicating whether this script is currently running in CI

    Returns:
        None
    """
    # Edit the run-whist-client.sh to make the client quit after the experiment is over
    print(f"Setting the experiment duration to {testing_time}s...")
    command = f"sed -i 's/timeout 240s/timeout {testing_time}s/g' ~/whist/mandelboxes/development/client/run-whist-client.sh"
    pexpect_process.sendline(command)
    wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)

    print(f"Building the dev client mandelbox in {cmake_build_type} mode ...")
    command = f"cd ~/whist/mandelboxes && ./build.sh development/client --{cmake_build_type} | tee ~/client_mandelbox_build.log"
    pexpect_process.sendline(command)
    wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)
    print("Finished building the dev client mandelbox on the EC2 instance")


def run_client_on_instance(pexpect_process, json_data, simulate_scrolling):
    """
    Run the Whist dev client (development/client mandelbox) on a remote machine accessible via
    a SSH connection within a pexpect process.

    The function assumes that the pexpect_process process has already successfully established
    a SSH connection to the host, that the Whist repository has already been cloned, and that
    the browsers/chrome mandelbox has already been built. Further, the host service must be
    already running on the remote machine.

    Args:
        pexpect_process: The Pexpect process created with pexpect.spawn(...) and to be used
                            to interact with the remote machine
        pexpect_prompt: The bash prompt printed by the shell on the remote machine when it is
                            ready to execute a command
        simulate_scrolling: A boolean controlling whether the client should simulate scrolling
                            as part of the test.

    Returns:
        client_docker_id: The Docker ID of the container running the Whist dev client
                            (development/client mandelbox) on the remote machine
    """
    print("Running the dev client mandelbox, and connecting to the server!")
    command = f"cd ~/whist/mandelboxes && ./run.sh development/client --json-data='{json.dumps(json_data)}'"
    pexpect_process.sendline(command)

    # Need to wait for special mandelbox prompt ":/#". running_in_ci must always be set to True in this case.
    client_mandelbox_output = wait_until_cmd_done(
        pexpect_process, ":/#", running_in_ci=True, return_output=True
    )
    client_docker_id = client_mandelbox_output[-2].replace(" ", "")
    print(f"Whist dev client started on EC2 instance, on Docker container {client_docker_id}!")

    if simulate_scrolling:
        # Sleep for sometime so that the webpage can load.
        time.sleep(5)
        print("Simulating the mouse scroll events in the client")
        command = "python3 /usr/share/whist/mouse_events.py"
        pexpect_process.sendline(command)
        wait_until_cmd_done(pexpect_process, ":/#", running_in_ci=True)

    return client_docker_id
