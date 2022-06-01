#!/usr/bin/env python3

import os, sys, json

from helpers.setup.network_tools import (
    restore_network_conditions,
)

from helpers.common.pexpect_tools import (
    wait_until_cmd_done,
    expression_in_pexpect_output,
    get_command_exit_code,
)

from helpers.common.ssh_tools import (
    attempt_ssh_connection,
    reboot_instance,
)

from helpers.setup.instance_setup_tools import (
    install_and_configure_aws,
    clone_whist_repository,
    run_host_setup,
    prune_containers_if_needed,
    prepare_instance_for_host_setup,
)
from helpers.common.git_tools import (
    get_remote_whist_github_sha,
    get_whist_github_sha,
)
from helpers.common.timestamps_and_exit_tools import (
    printyellow,
    exit_with_error,
)

from helpers.common.constants import (
    MANDELBOX_BUILD_MAX_RETRIES,
    username,
)

# Add the current directory to the path no matter where this is called from
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
        args_dict (multiprocessing.managers.DictProxy): A dictionary containing the configs needed to access the remote machine
                                                        and get a Whist dev client ready for execution

    Returns:
        None
    """

    client_hostname = args_dict["client_hostname"]
    ssh_key_path = args_dict["ssh_key_path"]
    client_log_filepath = args_dict["client_log_filepath"]
    pexpect_prompt_client = args_dict["pexpect_prompt_client"]
    github_token = args_dict["github_token"]
    use_two_instances = args_dict["use_two_instances"]
    testing_time = args_dict["testing_time"]
    cmake_build_type = args_dict["cmake_build_type"]
    skip_git_clone = args_dict["skip_git_clone"]
    skip_host_setup = args_dict["skip_host_setup"]

    client_log = open(client_log_filepath, "w")
    client_cmd = f"ssh {username}@{client_hostname} -i {ssh_key_path}"

    # If we are using the same instance for client and server, all the operations in this
    # if-statement have already been done by server_setup_process
    if use_two_instances:
        # Initiate the SSH connections with the client instance
        print("Initiating the SETUP SSH connection with the client AWS instance...")
        hs_process = attempt_ssh_connection(
            client_cmd,
            client_log,
            pexpect_prompt_client,
        )

        # Restore network conditions in case a previous run failed / was canceled before restoring the normal conditions.
        restore_network_conditions(hs_process, pexpect_prompt_client)

        print("Running pre-host-setup on the instance...")
        prepare_instance_for_host_setup(hs_process, pexpect_prompt_client)

        print("Configuring AWS credentials on client instance...")
        install_and_configure_aws(
            hs_process,
            pexpect_prompt_client,
        )

        prune_containers_if_needed(hs_process, pexpect_prompt_client)

        if skip_git_clone == "false":
            clone_whist_repository(github_token, hs_process, pexpect_prompt_client)
        else:
            print("Skipping git clone whisthq/whist repository on client instance.")

        # Ensure that the commit hash on client matches the one on the runner
        client_sha = get_remote_whist_github_sha(hs_process, pexpect_prompt_client)
        local_sha = get_whist_github_sha()
        if client_sha != local_sha:
            exit_with_error(
                f"Commit mismatch between client instance ({client_sha}) and E2E runner ({local_sha})"
            )

        if skip_host_setup == "false":
            run_host_setup(hs_process, pexpect_prompt_client)
        else:
            print("Skipping host-setup on server instance.")

        # Reboot and wait for it to come back up
        print("Rebooting the client EC2 instance (required after running the host-setup)...")
        hs_process = reboot_instance(
            hs_process,
            client_cmd,
            client_log,
            pexpect_prompt_client,
        )

        hs_process.kill(0)

    # Build the dev client
    print("Initiating the BUILD ssh connection with the client AWS instance...")
    client_pexpect_process = attempt_ssh_connection(
        client_cmd,
        client_log,
        pexpect_prompt_client,
    )
    build_client_on_instance(
        client_pexpect_process, pexpect_prompt_client, testing_time, cmake_build_type
    )
    client_pexpect_process.kill(0)
    client_log.close()


def build_client_on_instance(pexpect_process, pexpect_prompt, testing_time, cmake_build_type):
    """
    Build the Whist protocol client (development/client mandelbox) on a remote machine accessible
    via a SSH connection within a pexpect process.

    The function assumes that the pexpect_process process has already successfully established a
    SSH connection to the host, and that the Whist repository has already been cloned.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn):  The Pexpect process created with pexpect.spawn(...) and to
                                                    be used to interact with the remote machine
        pexpect_prompt (str):   The bash prompt printed by the shell on the remote machine when it is ready to
                                execute a command
        testing_time (int): The amount of time to leave the connection open between the client and the server
                            (when the client is started) before shutting it down
        cmake_build_type (str): A string identifying whether to build the protocol in release, debug, metrics,
                                or any other Cmake build mode that will be introduced later.

    Returns:
        None
    """
    # Edit the run-whist-client.sh to make the client quit after the experiment is over
    print(f"Setting the experiment duration to {testing_time}s...")
    command = f"sed -i 's/timeout 240s/timeout {testing_time}s/g' ~/whist/mandelboxes/development/client/run-whist-client.sh"
    pexpect_process.sendline(command)
    wait_until_cmd_done(pexpect_process, pexpect_prompt)

    command = f"cd ~/whist/mandelboxes && ./build.sh development/client --{cmake_build_type} | tee ~/client_mandelbox_build.log"
    success_msg = "All images built successfully!"

    for retry in range(MANDELBOX_BUILD_MAX_RETRIES):
        print(
            f"Building the dev client mandelbox in {cmake_build_type} mode (retry {retry+1}/{MANDELBOX_BUILD_MAX_RETRIES})..."
        )
        pexpect_process.sendline(command)
        build_client_output = wait_until_cmd_done(
            pexpect_process, pexpect_prompt, return_output=True
        )
        build_client_exit_code = get_command_exit_code(pexpect_process, pexpect_prompt)

        # Check if build succeeded
        if build_client_exit_code == 0 and expression_in_pexpect_output(
            success_msg, build_client_output
        ):
            print("Finished building the dev client mandelbox on the EC2 instance")
            break
        else:
            printyellow("Could not build the development/client mandelbox on the client instance!")
        if retry == MANDELBOX_BUILD_MAX_RETRIES - 1:
            # If building the development/client mandelbox fails too many times, trigger a fatal error.
            exit_with_error(
                f"Building the development/client mandelbox failed {MANDELBOX_BUILD_MAX_RETRIES} times. Giving up now!"
            )


def run_client_on_instance(pexpect_process, json_data, simulate_scrolling):
    """
    Run the Whist dev client (development/client mandelbox) on a remote machine accessible via
    a SSH connection within a pexpect process.

    The function assumes that the pexpect_process process has already successfully established
    a SSH connection to the host, that the Whist repository has already been cloned, and that
    the browsers/chrome mandelbox has already been built. Further, the host-service must be
    already running on the remote machine.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn):  The Pexpect process created with pexpect.spawn(...) and to
                                                    be used to interact with the remote machine
        pexpect_prompt (str):   The bash prompt printed by the shell on the remote machine when it is
                                ready to execute a command
        simulate_scrolling (int):  Number of rounds of scrolling.

    Returns:
        client_docker_id (str): The Docker ID of the container running the Whist dev client
                          (development/client mandelbox) on the remote machine
    """
    print("Running the dev client mandelbox, and connecting to the server!")
    command = f"cd ~/whist/mandelboxes && ./run.sh development/client --json-data='{json.dumps(json_data)}'"
    pexpect_process.sendline(command)

    # Need to wait for special mandelbox prompt ":/#". prompt_printed_twice must always be set to False in this case.
    client_mandelbox_output = wait_until_cmd_done(
        pexpect_process, ":/#", prompt_printed_twice=False, return_output=True
    )
    client_docker_id = client_mandelbox_output[-2].replace(" ", "")
    print(f"Whist dev client started on EC2 instance, on Docker container {client_docker_id}!")

    if simulate_scrolling > 0:
        # Launch the script to simulate the scrolling in the background
        command = f"(nohup /usr/share/whist/simulate_mouse_scrolling.sh {simulate_scrolling} > /var/log/whist/simulated_scrolling.log 2>&1 & )"
        pexpect_process.sendline(command)
        wait_until_cmd_done(pexpect_process, ":/#", prompt_printed_twice=False)

    return client_docker_id
