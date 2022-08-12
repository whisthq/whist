#!/usr/bin/env python3

import os, sys, json
import pexpect

from e2e_helpers.whist_run_steps import (
    shutdown_and_wait_server_exit,
)

from e2e_helpers.common.timestamps_and_exit_tools import (
    exit_with_error,
    printformat,
)

from e2e_helpers.common.constants import (
    SESSION_ID_LEN,
    username,
    aws_timeout_seconds,
    running_in_ci,
)

from e2e_helpers.aws.boto3_tools import (
    terminate_or_stop_aws_instance,
)

from e2e_helpers.setup.network_tools import (
    restore_network_conditions,
)
from e2e_helpers.common.remote_executor import RemoteExecutor

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


def get_session_id(remote_executor, role, session_id_filename="/whist/resourceMappings/session_id"):
    """
    Get the protocol session id (if it is set)

    Args:
        remote_executor (RemoteExecutor):  The executor object to be used to run commands on the remote instance
        role (str): Controls whether to extract the `server` logs or the `client` logs
        session_id_filename (str): The path to the file on the Docker container with the protocol session id

    Returns:
        On success:
            session_id (str): The protocol session id
        On failure:
            empty string
    """

    # Check if the session_id file exists
    remote_executor.run_command(f"test -f {session_id_filename}")
    session_id_is_set = remote_executor.get_exit_code() == 0

    if not session_id_is_set:
        print(f"{role} session id is not set")
        return ""

    remote_executor.run_command(f'echo "$(cat {session_id_filename})"')

    if (
        len(remote_executor.pexpect_output) != 3
        or len(remote_executor.pexpect_output[1]) != SESSION_ID_LEN
    ):
        print(remote_executor.pexpect_output)
        printformat(f"Could not parse the {role} session id!", "yellow")
        return ""

    return remote_executor.pexpect_output[1]


def extract_logs_from_mandelbox(
    remote_executor,
    docker_id,
    ssh_key_path,
    public_ip,
    e2e_logs_folder_name,
    log_filename,
    session_id,
    role,
):
    """
    Extract the logs related to the run of the Whist server/client mandelbox (i.e. browsers/chrome)
    on a remote machine. This should be called after the client->server connection has terminated,
    but before the server/client container is stopped/destroyed.

    Args:
        remote_executor (RemoteExecutor):  The executor object to be used to run commands on the remote instance
        docker_id (str):    The Docker ID of the container running the Whist server/client
                            (browsers/chrome or development/client mandelbox) on the remote machine
        ssh_key_path (str): The path (on the machine where this script is run) to the file storing
                            the public RSA key used for SSH connections
        public_ip (str): The host name of the remote machine where the server/client was running on
        e2e_logs_folder_name (str):    The path to the folder (on the machine where this script is run)
                                        where to store the logs
        log_filename (file):    The path to the file to use for logging the terminal output from the shell
                                process used to download the logs
        session_id (str): The protocol session id (if set), or an empty string (otherwise)
        role (str): Controls whether to extract the `server` logs or the `client` logs

    Returns:
        None
    """
    command = f"rm -rf ~/e2e_logs/{role}; mkdir -p ~/e2e_logs/{role}"
    remote_executor.run_command(command)

    logfiles = [
        # Log file with data for plotting only exists when running in metrics mode
        "/usr/share/whist/plot_data.json",
        # Log file below will only exist on the client container when a >0 simulated_scrolling argument is used
        "/usr/share/whist/simulated_scrolling.log",
        # Var Logs without session_id
        *[
            os.path.join("/var/log/whist", session_id, f"{component}-{log_type}.log")
            for component in ["startup", "update_xorg_conf"]
            for log_type in ["err", "out"]
        ],
        # Var logs with session_id (which might not be set)
        *[
            os.path.join("/var/log/whist", session_id, f"{component}-{log_type}.log")
            for component in [
                "audio",
                "display",
                "main",
                f"protocol_{role}",
                "teleport_drag_drop",
                "whist_application",
            ]
            for log_type in ["err", "out"]
        ],
    ]
    if role == "server":
        # Download Chrome preferences if they exist (only first file should exist)
        logfiles.append("/home/whist/.config/google-chrome/Default/Preferences")
        logfiles.append("/home/whist/.config/google-chrome/Default/History")

    for file_path in logfiles:
        command = f"docker cp {docker_id}:{file_path} ~/e2e_logs/{role}/"
        remote_executor.run_command(command, ignore_exit_codes=True)

    # Move the network conditions log to the e2e_logs folder, so that it is downloaded
    # to the machine running this script along with the other logs
    if role == "client":
        command = "mv ~/network_conditions.log ~/e2e_logs/client/network_conditions.log"
        remote_executor.run_command(command, ignore_exit_codes=True)
    # Extract URLs from history, to ensure that the desired websites were opened
    else:
        command = "strings ~/e2e_logs/server/History | grep http > ~/e2e_logs/server/history.log && rm ~/e2e_logs/server/History"
        remote_executor.run_command(command, ignore_exit_codes=True)

    # Download all the mandelbox logs from the AWS machine
    command = (
        f"scp -r -i {ssh_key_path} {username}@{public_ip}:~/e2e_logs/{role} {e2e_logs_folder_name}"
    )

    logfile = open(log_filename, "a+")
    local_process = pexpect.spawn(command, timeout=aws_timeout_seconds, logfile=logfile.buffer)
    local_process.expect(["\$", pexpect.EOF])
    local_process.kill(0)


def complete_experiment_and_save_results(
    server_public_ip,
    server_private_ip,
    server_instance_id,
    server_docker_id,
    server_log_filename,
    server_metrics_file,
    region_name,
    existing_server_instance_id,
    server_executor,
    server_hs_executor,
    client_public_ip,
    client_private_ip,
    client_instance_id,
    client_docker_id,
    client_log_filename,
    client_metrics_file,
    existing_client_instance_id,
    client_executor,
    client_hs_executor,
    ssh_key_path,
    boto3client,
    use_two_instances,
    leave_instances_on,
    network_conditions,
    e2e_logs_folder_name,
    experiment_metadata,
    metadata_filename,
    timestamps,
):
    """
    This function performs the teardown and cleanup required at the end of a E2E streaming test. This
    involves:
    - shutting down the WhistClient and WhistServer
    - checking  that the WhistServer exits without hanging
    - shutting down the client (development/client) and server (browsers/chrome) mandelboxes
    - restoring the default network conditions on the client instance,
    - extracting the experiment logs from the instance(s)
    - stopping or terminating the EC2 instance(s)
    - update the metadata file with the experiment results
    - determining whether the E2E test succeeded or failed

    Args:
        server_public_ip (str):  The host name of the remote machine where the server was running on
        server_public_ip (str):  The private IP of the remote machine where the server was running on
        server_instance_id (str):   The ID of the AWS EC2 instance running the server
        server_docker_id (str): The ID of the Docker container running the server (browsers/chrome) mandelbox
        server_log_filename (file):  The path to the file to be used to dump the server-side monitoring logs
        server_metrics_file (file): The filepath to the file (that we expect to see) containing the server metrics.
                                    We will use this filepath to check that the file exists.
        region_name (str): The AWS region hosting the EC2 instance(s)
        existing_server_instance_id (str): the ID of the pre-existing AWS EC2 instance that was used to run the test.
                                            This parameter is an empty string if we are not reusing existing instances
        server_executor (RemoteExecutor):   The executor object to be used to run commands on the server mandelbox
                                            on the server instance.
        server_hs_executor (RemoteExecutor):    The executor object to be used to interact with the host-service
                                                on the server instance.
        client_public_ip (str):  The host name of the remote machine where the client was running on
        client_private_ip (str):  The private IP of the remote machine where the client was running on
        client_instance_id (str):   The ID of the AWS EC2 instance running the client
        client_docker_id (str): The ID of the Docker container running the client (development/client) mandelbox
        client_log_filename (file):  The path to the file to be used to dump the client-side monitoring logs
        client_metrics_file (file): The filepath to the file (that we expect to see) containing the client metrics.
                                    We will use this filepath to check that the file exists.
        existing_client_instance_id (str): the ID of the pre-existing AWS EC2 instance that was used to run the test.
                                            This parameter is an empty string if we are not reusing existing instances
        client_executor (RemoteExecutor):   The executor object to be used to run commands on the client mandelbox
                                            on the server instance.
        client_hs_executor (RemoteExecutor):    The executor object to be used to interact with the host-service
                                                on the client instance.
        ssh_key_path (str): The path (on the machine where this script is run) to the file storing
                            the public RSA key used for SSH connections
        boto3client (botocore.client): The Boto3 client to use to talk to the AWS console
        use_two_instances (bool):   Whether the server and the client are running on two separate AWS instances
                                    (as opposed to the same instance)
        leave_instances_on (bool): Whether to leave the instance(s) running after the experiment is complete.
        network_conditions (str):   The network conditions used on the client instanceduring the experiment.
                                    This string is set to 'none' if no artificial degradations were applied
                                    to the network on the client instance.
        e2e_logs_folder_name (str):    The path to the folder (on the machine where this script is run)
                                        where to store the logs
        experiment_metadata (dict): The dictionary containing the experiment metadata
        metadata_filename (str): The name of the file to save the updated experiment metadata in json format
        timestamps (Timestamps):  The Timestamps object containing the timestamps from the major E2E events

    Returns:
        None
    """

    # 1- Restore un-degradated network conditions in case the instance is reused later on.
    # Do this before downloading the logs to prevent the download from taking a long time.
    if network_conditions != "normal":
        # Get new SSH connection because current ones are connected to the mandelboxes' bash,
        # and we cannot exit them until we have copied over the logs
        client_restore_net_executor = RemoteExecutor(
            client_public_ip,
            client_private_ip,
            ssh_key_path,
            client_log_filename,
        )
        restore_network_conditions(client_restore_net_executor)
        client_restore_net_executor.destroy()

    timestamps.add_event("Restoring un-degraded network conditions")

    # 2 - Extracting the session IDs, if they are set
    server_session_id = get_session_id(server_executor, "server")
    client_session_id = get_session_id(client_executor, "client")
    timestamps.add_event("Extracting protocol session IDs")

    # 3- Quit the server and check whether it shuts down gracefully or whether it hangs
    server_hang_detected = False
    server_shutdown_desired_message = "Both whist-application and WhistServer have exited."
    if shutdown_and_wait_server_exit(
        server_executor, server_session_id, server_shutdown_desired_message
    ):
        print("Server has exited gracefully.")
    else:
        print("Server has not exited gracefully!")
        server_hang_detected = True

    timestamps.add_event("Shutting down the server and checking for potential server hang")

    # 4- Extract the client/server protocol logs from the two Docker containers
    print("Initiating LOG GRABBING ssh connection(s) with the AWS instance(s)...")

    log_grabber_server_executor = RemoteExecutor(
        server_public_ip,
        server_private_ip,
        ssh_key_path,
        server_log_filename,
    )

    log_grabber_client_executor = RemoteExecutor(
        client_public_ip,
        client_private_ip,
        ssh_key_path,
        client_log_filename,
    )

    extract_logs_from_mandelbox(
        log_grabber_server_executor,
        server_docker_id,
        ssh_key_path,
        server_public_ip,
        e2e_logs_folder_name,
        server_log_filename,
        server_session_id,
        role="server",
    )
    extract_logs_from_mandelbox(
        log_grabber_client_executor,
        client_docker_id,
        ssh_key_path,
        client_public_ip,
        e2e_logs_folder_name,
        client_log_filename,
        client_session_id,
        role="client",
    )

    timestamps.add_event("Extracting the mandelbox logs")

    # 5- Clean up the instance(s) by stopping all docker containers and quitting the host-service.
    # Exit the server/client mandelboxes
    server_executor.set_mandelbox_mode(False)
    client_executor.set_mandelbox_mode(False)
    server_executor.run_command("exit", ignore_exit_codes=True)
    client_executor.run_command("exit", ignore_exit_codes=True)

    # Stop and delete any leftover Docker containers
    command = "docker stop $(docker ps -aq) && docker rm $(docker ps -aq)"
    server_executor.run_command(command, ignore_exit_codes=True)
    if use_two_instances:
        client_executor.run_command(command, ignore_exit_codes=True)

    # Terminate the host-service
    server_hs_executor.pexpect_process.sendcontrol("c")
    server_hs_executor.destroy()
    if use_two_instances:
        client_hs_executor.pexpect_process.sendcontrol("c")
        client_hs_executor.destroy()

    # Terminate all pexpect processes
    server_executor.destroy()
    client_executor.destroy()
    log_grabber_server_executor.destroy()
    log_grabber_client_executor.destroy()

    timestamps.add_event("Stopping all containers and closing connections to the instance(s)")

    # 7- Stop or terminate the AWS EC2 instance(s)
    if leave_instances_on == "false":
        # Terminate or stop AWS instance(s)
        terminate_or_stop_aws_instance(
            boto3client, server_instance_id, server_instance_id != existing_server_instance_id
        )
        if use_two_instances:
            terminate_or_stop_aws_instance(
                boto3client,
                client_instance_id,
                client_instance_id != existing_client_instance_id,
            )
    elif running_in_ci:
        # Save instance IDs to file for reuse by later runs
        with open("instances_left_on.txt", "w") as instances_file:
            instances_file.write(f"{region_name}\n")
            instances_file.write(f"{server_instance_id}\n")
            if client_instance_id != server_instance_id:
                instances_file.write(f"{client_instance_id}\n")

    print("Instance successfully stopped/terminated, goodbye")

    timestamps.add_event("Stopping/terminating instance(s)")

    # 8- Check if either of the WhistServer/WhistClient failed to start, or whether the client failed
    # to connect to the server. If so, add the error to the metadata, and exit with an error code (-1).

    # The server_metrics_file (server.log) and the client_metrics_file (client.log) fail to exist if
    # the WhistServer/WhistClient did not start
    experiment_metadata["server_failure"] = not os.path.isfile(server_metrics_file)
    experiment_metadata["client_failure"] = not os.path.isfile(client_metrics_file)

    # A test without connection errors will produce a client log that is well over 500 lines.
    # This is a heuristic that works surprisingly well under the current protocol setup.
    # If the connection failed, trigger an error.
    experiment_metadata["client_server_connection_failure"] = (
        not experiment_metadata["client_failure"]
        and len(open(client_metrics_file).readlines()) <= 500
    )
    experiment_metadata["server_hang_detected"] = (
        server_hang_detected and not experiment_metadata["server_failure"]
    )

    # 9- Update metadata file with any new metadata that we added
    with open(metadata_filename, "w") as metadata_file:
        json.dump(experiment_metadata, metadata_file)

    # 10- Print time breakdown of the experiment
    timestamps.print_timestamps()

    # 11- Print error message and exit with error if needed
    for cause, message in {
        "server_failure": "Failed to run WhistServer",
        "client_failure": "Failed to run WhistClient",
        "client_server_connection_failure": "Whist Client failed to connect to the server",
        "server_hang_detected": "WhistServer hang detected",
    }.items():
        if experiment_metadata[cause]:
            exit_with_error(f"{message}! Exiting with error. Check the logs for more details.")
