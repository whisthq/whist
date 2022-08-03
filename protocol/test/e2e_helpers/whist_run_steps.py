#!/usr/bin/env python3

import os, sys
import base64, json, zlib

from e2e_helpers.common.pexpect_tools import (
    expression_in_pexpect_output,
    wait_until_cmd_done,
    get_command_exit_code,
)

from e2e_helpers.common.ssh_tools import (
    attempt_ssh_connection,
    reboot_instance,
)

from e2e_helpers.setup.instance_setup_tools import (
    install_and_configure_aws,
    clone_whist_repository,
    run_host_setup,
    prune_containers_if_needed,
    prepare_instance_for_host_setup,
)

from e2e_helpers.setup.network_tools import (
    restore_network_conditions,
)

from e2e_helpers.common.git_tools import (
    get_remote_whist_github_sha,
    get_whist_github_sha,
)
from e2e_helpers.common.timestamps_and_exit_tools import (
    printformat,
    exit_with_error,
)

from e2e_helpers.common.constants import (
    MANDELBOX_BUILD_MAX_RETRIES,
    username,
)

# Add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


def setup_process(role, args_dict):
    """
    Prepare a remote Ubuntu machine to host the Whist server/client. This entails:
    - opening a SSH connection to the machine
    - (CLIENT only): restoring the default network conditions, if some artificial network
          degradation was previously applied on the instance
    - For the SERVER (or a CLIENT not running on the same machine as the server):
        - installing the AWS CLI and setting up the AWS credentials
        - pruning the docker containers if we are running out of space on disk
        - cloning the current branch of the Whist repository if needed
        - launching the Whist host setup if needed
    - building the server/client mandelbox

    In case of error, this function makes the process running it exit with exitcode -1.

    Args:
        role (str): The role of the instance we are setting up (server or client)
        args_dict (multiprocessing.managers.DictProxy): A dictionary containing the configs needed to access the remote
                                                        machine and get a Whist server/client ready for execution

    Returns:
        None
    """
    hostname = args_dict["server_hostname"] if role == "server" else args_dict["client_hostname"]
    ssh_key_path = args_dict["ssh_key_path"]
    log_filepath = (
        args_dict["server_log_filepath"] if role == "server" else args_dict["client_log_filepath"]
    )
    pexpect_prompt = (
        args_dict["pexpect_prompt_server"]
        if role == "server"
        else args_dict["pexpect_prompt_client"]
    )
    use_two_instances = args_dict["use_two_instances"]
    testing_time = args_dict["testing_time"]
    github_token = args_dict["github_token"]
    cmake_build_type = args_dict["cmake_build_type"]
    skip_git_clone = args_dict["skip_git_clone"]
    skip_host_setup = args_dict["skip_host_setup"]

    ssh_cmd = (
        f"ssh {username}@{hostname} -i {ssh_key_path} -o TCPKeepAlive=yes -o ServerAliveInterval=15"
    )

    logfile = open(log_filepath, "w")

    # Initiate the SSH connections with the instance
    print(f"Initiating the SETUP ssh connection with the {role} AWS instance...")
    hs_process = attempt_ssh_connection(ssh_cmd, logfile, pexpect_prompt)

    if role == "client":
        # Restore network conditions in case a previous run failed / was canceled before restoring the normal conditions.
        restore_network_conditions(hs_process, pexpect_prompt)

    if role == "server" or (role == "client" and use_two_instances):
        print(f"Running pre-host-setup on the {role} instance...")
        prepare_instance_for_host_setup(hs_process, pexpect_prompt)

        print(f"Configuring AWS credentials on {role} instance...")
        install_and_configure_aws(hs_process, pexpect_prompt)

        prune_containers_if_needed(hs_process, pexpect_prompt)

        if skip_git_clone == "false":
            clone_whist_repository(github_token, hs_process, pexpect_prompt)
        else:
            print(f"Skipping git clone whisthq/whist repository on {role} instance.")
            # Delete any changes to the repo from previous runs (e.g. testing time settings)
            hs_process.sendline("cd ~/whist ; git reset --hard ; cd")
            wait_until_cmd_done(hs_process, pexpect_prompt)

        # Ensure that the commit hash on the remote instance matches the one on the runner
        instance_sha = get_remote_whist_github_sha(hs_process, pexpect_prompt)
        local_sha = get_whist_github_sha()
        if instance_sha != local_sha:
            exit_with_error(
                f"Commit mismatch between {role} instance ({instance_sha}) and E2E runner ({local_sha}). This can happen when re-running a CI workflow after having pushed new commits."
            )

        if skip_host_setup == "false":
            run_host_setup(hs_process, pexpect_prompt)
        else:
            print("Skipping host setup on server instance.")

        # Reboot and wait for it to come back up
        print(f"Rebooting the {role} EC2 instance (required after running the host-setup)...")
        hs_process = reboot_instance(hs_process, ssh_cmd, logfile, pexpect_prompt)
        hs_process.kill(0)

    print(f"Done with the setup on the {role} instance.")

    print(f"Initiating the BUILD ssh connection with the {role} AWS instance...")
    build_process = attempt_ssh_connection(ssh_cmd, logfile, pexpect_prompt)
    build_mandelboxes_on_instance(hs_process, pexpect_prompt, cmake_build_type, role, testing_time)
    build_process.kill(0)

    logfile.close()


def get_mandelbox_name(role):
    """
    Get the default mandelbox name for the server (browsers/chrome) and the client (development/client)

    Args:
        role (str): Specifies whether we need the server or client mandelbox

    Returns:
        mandelbox_name (str): The name of the default mandelbox
    """
    return "browsers/chrome" if role == "server" else "development/client"


def build_mandelboxes_on_instance(
    pexpect_process, pexpect_prompt, cmake_build_type, role, testing_time
):
    """
    Build the Whist mandelboxes (browsers/chrome for the server, development/client for the client) on a
    remote machine accessible via a SSH connection within a pexpect process.

    The function assumes that the pexpect_process process has already successfully established a
    SSH connection to the host, and that the Whist repository has already been cloned.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn):  The Pexpect process created with pexpect.spawn(...) and to be used to
                                                    interact with the remote machine
        pexpect_prompt (str):   The bash prompt printed by the shell on the remote machine when
                                it is ready to execute a command
        cmake_build_type (str): A string identifying whether to build the protocol in release,
                                debug, metrics, or any other Cmake build mode that will be introduced later.
        role (str): Specifies whether we need to build the server or client mandelbox
        testing_time (int): The amount of time to leave the connection open between the client and the server
                            (when the client is started) before shutting it down

    Returns:
        None
    """
    if role == "client":
        # Edit the run-whist-client.sh to make the client quit after the experiment is over
        print(f"Setting the experiment duration to {testing_time}s...")
        command = f"sed -i 's/sleep 240/sleep {testing_time}/g' ~/whist/mandelboxes/development/client/run-whist-client.sh"
        pexpect_process.sendline(command)
        wait_until_cmd_done(pexpect_process, pexpect_prompt)

    mandelbox_name = get_mandelbox_name(role)
    print(f"Building the {mandelbox_name} mandelbox on the {role} instance...")

    command = f"cd ~/whist/mandelboxes && ./build.sh {mandelbox_name} --{cmake_build_type} | tee ~/{role}_mandelbox_build.log"
    success_msg = "All images built successfully!"
    docker_tar_io_eof_error = "io: read/write on closed pipe"
    docker_connect_error = "error during connect: Post"

    for retry in range(MANDELBOX_BUILD_MAX_RETRIES):
        print(
            f"Building the {mandelbox_name} mandelbox on the {role} instance with the {cmake_build_type} protocol (retry {retry+1}/{MANDELBOX_BUILD_MAX_RETRIES})..."
        )

        # Attempt to build the mandelbox and grab the output + the exit code
        pexpect_process.sendline(command)
        build_mandelbox_output = wait_until_cmd_done(
            pexpect_process, pexpect_prompt, return_output=True
        )
        build_mandelbox_exit_code = get_command_exit_code(pexpect_process, pexpect_prompt)

        # Check if the build succeeded
        if build_mandelbox_exit_code == 0 and expression_in_pexpect_output(
            success_msg, build_mandelbox_output
        ):
            print(f"Finished building the {mandelbox_name} ({role}) mandelbox on the EC2 instance")
            break
        else:
            printformat(
                f"Could not build the {mandelbox_name} mandelbox on the {role} instance!", "yellow"
            )
            if expression_in_pexpect_output(
                docker_tar_io_eof_error, build_mandelbox_output
            ) or expression_in_pexpect_output(docker_connect_error, build_mandelbox_output):
                print("Detected docker build issue. Attempting to fix by restarting docker!")
                pexpect_process.sendline("sudo service docker restart")
                wait_until_cmd_done(pexpect_process, pexpect_prompt)
        if retry == MANDELBOX_BUILD_MAX_RETRIES - 1:
            # If building the mandelbox fails too many times, trigger a fatal error.
            exit_with_error(
                f"Building the {mandelbox_name} mandelbox failed {MANDELBOX_BUILD_MAX_RETRIES} times. Giving up now!"
            )


def run_mandelbox_on_instance(pexpect_process, role, json_data=None, simulate_scrolling=0):
    """
    Run the server/client mandelbox on a remote machine accessible via a SSH connection within a pexpect process.

    The function assumes that the pexpect_process process has already successfully established
    a SSH connection to the host, that the Whist repository has already been cloned, and that
    the browsers/chrome mandelbox has already been built. Further, the host service must be
    already running on the remote machine.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn):  The Pexpect process created with pexpect.spawn(...) and to be used to
                                                    interact with the remote machine
        role (str): Specifies whether we need to run the server or client mandelbox
        json_data (str):    A dictionary containing the IP, AES KEY, and port mappings that are needed by the client to
                            successfully connect to the Whist server. This argument will be None and is ignored when
                            building the server.
        simulate_scrolling (int):  Number of rounds of scrolling to run on the client mandelbox. This argument is ignored
                                    when building the server.

    Returns:
        docker_id (str):    The Docker ID of the container running the Whist mandelbox on the remote machine
        Server only:
            json_data (str):    A dictionary containing the IP, AES KEY, and port mappings that are needed by
                                the client to successfully connect to the Whist server.
    """
    mandelbox_name = get_mandelbox_name(role)
    json_data_from_server = ""
    if role == "server":
        print(f"Running the {role} mandelbox, and extracting the info needed to connect to it!")
    else:
        print(f"Running the {role} mandelbox, and connecting to the server!")
        # Compress JSON data using gunzip
        zlib_compressor = zlib.compressobj(level=zlib.Z_BEST_COMPRESSION, wbits=16 + zlib.MAX_WBITS)
        compressed_json_data = base64.b64encode(
            zlib_compressor.compress(json.dumps(json_data).encode("utf-8"))
            + zlib_compressor.flush()
        ).decode("utf-8")
        json_data_from_server = f"--json-data='{compressed_json_data}'"

    command = f"cd ~/whist/mandelboxes && ./run.sh {mandelbox_name} --new-user-config-token {json_data_from_server} | tee ~/server_mandelbox_run.log"
    pexpect_process.sendline(command)

    # Need to wait for special mandelbox prompt ":/#". prompt_printed_twice must always be set to False in this case.
    mandelbox_output = wait_until_cmd_done(
        pexpect_process, ":/#", prompt_printed_twice=False, return_output=True
    )
    docker_id = mandelbox_output[-2].replace(" ", "")
    print(f"Whist {role} started on EC2 instance, on Docker container {docker_id}!")

    if role == "server":
        # Retrieve connection configs from server
        json_data = {}
        for line in mandelbox_output:
            if "linux/macos" in line:
                config_vals = line.strip().split()
                server_addr = config_vals[2]
                port_mappings = config_vals[3]
                aes_key = config_vals[5]
                json_data["dev_client_server_ip"] = server_addr
                # Remove leading '-p' chars
                json_data["dev_client_server_port_mappings"] = port_mappings[2:]
                json_data["dev_client_server_aes_key"] = aes_key
        return docker_id, json_data
    else:
        if simulate_scrolling > 0:
            # Launch the script to simulate the scrolling in the background
            print(f"Starting the scrolling simulation (with {simulate_scrolling} rounds)!")
            command = f"(nohup /usr/share/whist/simulate_mouse_scrolling.sh {simulate_scrolling} > /usr/share/whist/simulated_scrolling.log 2>&1 & )"
            pexpect_process.sendline(command)
            wait_until_cmd_done(pexpect_process, ":/#", prompt_printed_twice=False)

        return docker_id


def shutdown_and_wait_server_exit(pexpect_process, session_id, exit_confirm_exp):
    """
    Initiate shutdown and wait for server exit to see if the server hangs or exits gracefully

    Args:
        pexpect_process (pexpect.pty_spawn.spawn):  Server pexpect process - MUST BE AFTER DOCKER COMMAND WAS RUN - otherwise
                                                    behavior is undefined
        session_id (str): The protocol session id (if set), or an empty string (otherwise)
        exit_confirm_exp (str): Target expression to expect on a graceful server exit

    Returns:
        server_has_exited (bool): A boolean set to True if server has exited gracefully, false otherwise
    """
    # Shut down Chrome
    pexpect_process.sendline("pkill chrome")

    # We set prompt_printed_twice=False because the Docker bash does not print in color
    # (check wait_until_cmd_done docstring for more details about handling color bash stdout)
    wait_until_cmd_done(pexpect_process, ":/#", prompt_printed_twice=False)

    # Give WhistServer 20s to shutdown properly
    pexpect_process.sendline("sleep 20")
    wait_until_cmd_done(pexpect_process, ":/#", prompt_printed_twice=False)

    # Check the log to see if WhistServer shut down gracefully or if there was a server hang
    server_log_filepath = os.path.join("/var/log/whist", session_id, "main-out.log")
    pexpect_process.sendline(f"tail {server_log_filepath}")
    server_mandelbox_output = wait_until_cmd_done(
        pexpect_process, ":/#", prompt_printed_twice=False, return_output=True
    )
    server_has_exited = expression_in_pexpect_output(exit_confirm_exp, server_mandelbox_output)

    # Kill tail process
    pexpect_process.sendcontrol("c")
    return server_has_exited
