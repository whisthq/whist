#!/usr/bin/env python3

import os, sys, json
import pexpect

from helpers.whist_server_tools import shutdown_and_wait_server_exit

from helpers.common.ssh_tools import (
    attempt_ssh_connection,
    wait_until_cmd_done,
)

from helpers.aws.boto3_tools import (
    terminate_or_stop_aws_instance,
)

from helpers.setup.network_tools import (
    restore_network_conditions,
)

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


def extract_logs_from_mandelbox(
    pexpect_process,
    pexpect_prompt,
    docker_id,
    ssh_key_path,
    username,
    hostname,
    timeout_value,
    perf_logs_folder_name,
    log_grabber_log,
    running_in_ci,
    role,
):
    """
    Extract the logs related to the run of the Whist server/client mandelbox (i.e. browsers/chrome)
    on a remote machine. This should be called after the client->server connection has terminated,
    but before the server/client container is stopped/destroyed.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn):  The Pexpect process created with pexpect.spawn(...) and to
                                                    be used to interact with the remote machine
        pexpect_prompt (str):   The bash prompt printed by the shell on the remote machine when it is
                                ready to execute a command
        docker_id (str):    The Docker ID of the container running the Whist server/client
                            (browsers/chrome or development/client mandelbox) on the remote machine
        ssh_key_path (str): The path (on the machine where this script is run) to the file storing
                            the public RSA key used for SSH connections
        username (str): The username to be used on the remote machine (default is 'ubuntu')
        hostname (str): The host name of the remote machine where the server/client was running on
        timeout_value (int):    The amount of time (in seconds) to wait before timing out the attemps to
                                gain a SSH connection to the remote machine.
        perf_logs_folder_name (str):    The path to the folder (on the machine where this script is run)
                                        where to store the logs
        log_grabber_log (file): The file (already opened) to use for logging the terminal output from
                                the shell process used to download the logs
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI
        role (str): Controls whether to extract the `server` logs or the `client` logs

    Returns:
        None
    """
    command = f"rm -rf ~/perf_logs/{role}; mkdir -p ~/perf_logs/{role}"
    pexpect_process.sendline(command)
    wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)

    logfiles = [
        f"/usr/share/whist/{role}.log",
        "/usr/share/whist/teleport.log",
        "/usr/share/whist/display.log",
        # Var Logs!
        "/var/log/whist/audio-err.log",
        "/var/log/whist/audio-out.log",
        "/var/log/whist/display-err.log",
        "/var/log/whist/display-out.log",
        "/var/log/whist/entry-err.log",
        "/var/log/whist/entry-out.log",
        "/var/log/whist/update_xorg_conf-err.log",
        "/var/log/whist/update_xorg_conf-out.log",
        "/var/log/whist/protocol-err.log",
    ]
    for file_path in logfiles:
        command = f"docker cp {docker_id}:{file_path} ~/perf_logs/{role}/"
        pexpect_process.sendline(command)
        wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)

    if role == "client":
        command = "mv ~/network_conditions.log ~/perf_logs/client/network_conditions.log"
        pexpect_process.sendline(command)
        wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)

    # Download all the mandelbox logs from the AWS machine
    command = (
        f"scp -r -i {ssh_key_path} {username}@{hostname}:~/perf_logs/{role} {perf_logs_folder_name}"
    )

    local_process = pexpect.spawn(command, timeout=timeout_value, logfile=log_grabber_log.buffer)
    local_process.expect(["\$", pexpect.EOF])
    local_process.kill(0)


def complete_experiment_and_save_results(
    server_hostname,
    server_instance_id,
    server_docker_id,
    server_ssh_cmd,
    server_log,
    server_metrics_file,
    use_existing_server_instance,
    server_mandelbox_pexpect_process,
    server_hs_process,
    pexpect_prompt_server,
    client_hostname,
    client_instance_id,
    client_docker_id,
    client_ssh_cmd,
    client_log,
    client_metrics_file,
    use_existing_client_instance,
    client_mandelbox_pexpect_process,
    client_hs_process,
    pexpect_prompt_client,
    aws_timeout_seconds,
    ssh_connection_retries,
    username,
    ssh_key_path,
    boto3client,
    running_in_ci,
    use_two_instances,
    leave_instances_on,
    network_conditions,
    perf_logs_folder_name,
    experiment_metadata,
    metadata_filename,
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
        server_hostname (str):  The host name of the remote machine where the server was running on
        server_instance_id (str):   The ID of the AWS EC2 instance running the server
        server_docker_id (str): The ID of the Docker container running the server (browsers/chrome) mandelbox
        server_ssh_cmd (str):   The string containing the command to be used to open a SSH connection to the server EC2 instance
        server_log (file):  The file to be used to dump the server-side monitoring logs
        server_metrics_file (file): The filepath to the file (that we expect to see) containing the server metrics.
                                    We will use this filepath to check that the file exists.
        use_existing_server_instance (str): the ID of the pre-existing AWS EC2 instance that was used to run the test.
                                            This parameter is an empty string if we are not reusing existing instances
        server_mandelbox_pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process created with pexpect.spawn(...) and to
                                                                    be used to interact with the server mandelbox on the server instance.
        server_hs_process (pexpect.pty_spawn.spawn):    The Pexpect process created with pexpect.spawn(...) and to be used to
                                                        interact with the host-service on the server instance.
        pexpect_prompt_server (str):    The bash prompt printed by the shell on the remote server machine when it is ready
                                        to execute a command
        client_hostname (str):  The host name of the remote machine where the client was running on
        client_instance_id (str):   The ID of the AWS EC2 instance running the client
        client_docker_id (str): The ID of the Docker container running the client (development/client) mandelbox
        client_ssh_cmd (str):   The string containing the command to be used to open a SSH connection to the client EC2 instance
        client_log (file):  The file to be used to dump the client-side monitoring logs
        client_metrics_file (file): The filepath to the file (that we expect to see) containing the client metrics.
                                    We will use this filepath to check that the file exists.
        use_existing_client_instance (str): the ID of the pre-existing AWS EC2 instance that was used to run the test.
                                            This parameter is an empty string if we are not reusing existing instances
        client_mandelbox_pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process created with pexpect.spawn(...) and to
                                                                    be used to interact with the client mandelbox on the client instance.
        client_hs_process (pexpect.pty_spawn.spawn):    The Pexpect process created with pexpect.spawn(...) and to be used to
                                                        interact with the host-service on the client instance.
        pexpect_prompt_client (str):    The bash prompt printed by the shell on the remote client machine when it is ready
                                        to execute a command
        aws_timeout_seconds (int):  The amount of time (in seconds) to wait before timing out the attemps to
                                    gain a SSH connection to the remote machine.
        ssh_connection_retries (int): The number of times to retry if a SSH connection cannot be immediately established
        username (str): The username to use when opening a SSH connection to a remote AWS EC2 machine
        ssh_key_path (str): The path (on the machine where this script is run) to the file storing
                            the public RSA key used for SSH connections
        boto3client (botocore.client): The Boto3 client to use to talk to the AWS console
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI
        use_two_instances (bool):   Whether the server and the client are running on two separate AWS instances
                                    (as opposed to the same instance)
        leave_instances_on (bool): Whether to leave the instance(s) running after the experiment is complete.
        network_conditions (str):   The network conditions used on the client instanceduring the experiment.
                                    This string is set to 'none' if no artificial degradations were applied
                                    to the network on the client instance.
        perf_logs_folder_name (str):    The path to the folder (on the machine where this script is run)
                                        where to store the logs
        experiment_metadata (dict): The dictionary containing the experiment metadata
        metadata_filename (str): The name of the file to save the updated experiment metadata in json format

    Returns:
        On success: 0
        On failure: -1
    """

    # 1- Restore un-degradated network conditions in case the instance is reused later on.
    # Do this before downloading the logs to prevent the download from taking a long time.
    if network_conditions != "normal":
        # Get new SSH connection because current ones are connected to the mandelboxes' bash,
        # and we cannot exit them until we have copied over the logs
        client_restore_net_process = attempt_ssh_connection(
            client_ssh_cmd,
            aws_timeout_seconds,
            client_log,
            pexpect_prompt_client,
            ssh_connection_retries,
            running_in_ci,
        )
        restore_network_conditions(client_restore_net_process, pexpect_prompt_client, running_in_ci)
        client_restore_net_process.kill(0)

    # 2- Quit the server and check whether it shuts down gracefully or whether it hangs
    server_hang_detected = False
    server_shutdown_desired_message = "Both whist-application and WhistServer have exited."
    if shutdown_and_wait_server_exit(
        server_mandelbox_pexpect_process, server_shutdown_desired_message
    ):
        print("Server has exited gracefully.")
    else:
        print("Server has not exited gracefully!")
        server_hang_detected = True

    # 3- Extract the client/server protocol logs from the two Docker containers
    print("Initiating LOG GRABBING ssh connection(s) with the AWS instance(s)...")

    log_grabber_server_process = attempt_ssh_connection(
        server_ssh_cmd,
        aws_timeout_seconds,
        server_log,
        pexpect_prompt_server,
        ssh_connection_retries,
        running_in_ci,
    )

    log_grabber_client_process = log_grabber_server_process
    if use_two_instances:
        log_grabber_client_process = attempt_ssh_connection(
            client_ssh_cmd,
            aws_timeout_seconds,
            client_log,
            pexpect_prompt_client,
            ssh_connection_retries,
            running_in_ci,
        )

    extract_logs_from_mandelbox(
        log_grabber_server_process,
        pexpect_prompt_server,
        server_docker_id,
        ssh_key_path,
        username,
        server_hostname,
        aws_timeout_seconds,
        perf_logs_folder_name,
        server_log,
        running_in_ci,
        role="server",
    )
    extract_logs_from_mandelbox(
        log_grabber_client_process,
        pexpect_prompt_client,
        client_docker_id,
        ssh_key_path,
        username,
        client_hostname,
        aws_timeout_seconds,
        perf_logs_folder_name,
        client_log,
        running_in_ci,
        role="client",
    )

    # 4- Clean up the instance(s) by stopping all docker containers and quitting the host-service.
    # Exit the server/client mandelboxes
    server_mandelbox_pexpect_process.sendline("exit")
    wait_until_cmd_done(server_mandelbox_pexpect_process, pexpect_prompt_server, running_in_ci)
    client_mandelbox_pexpect_process.sendline("exit")
    wait_until_cmd_done(client_mandelbox_pexpect_process, pexpect_prompt_client, running_in_ci)

    # Stop and delete any leftover Docker containers
    command = "docker stop $(docker ps -aq) && docker rm $(docker ps -aq)"
    server_mandelbox_pexpect_process.sendline(command)
    wait_until_cmd_done(server_mandelbox_pexpect_process, pexpect_prompt_server, running_in_ci)
    if use_two_instances:
        client_mandelbox_pexpect_process.sendline(command)
        wait_until_cmd_done(client_mandelbox_pexpect_process, pexpect_prompt_client, running_in_ci)

    # Terminate the host-service
    server_hs_process.sendcontrol("c")
    server_hs_process.kill(0)
    if use_two_instances:
        client_hs_process.sendcontrol("c")
        client_hs_process.kill(0)

    # Terminate all pexpect processes
    server_mandelbox_pexpect_process.kill(0)
    client_mandelbox_pexpect_process.kill(0)
    log_grabber_server_process.kill(0)
    if use_two_instances:
        log_grabber_client_process.kill(0)

    # 5- Close all the log files
    server_log.close()
    client_log.close()

    # 6- Stop or terminate the AWS EC2 instance(s)
    if leave_instances_on == "false":
        # Terminate or stop AWS instance(s)
        terminate_or_stop_aws_instance(
            boto3client, server_instance_id, server_instance_id != use_existing_server_instance
        )
        if use_two_instances:
            terminate_or_stop_aws_instance(
                boto3client,
                client_instance_id,
                client_instance_id != use_existing_client_instance,
            )
    else:
        # Save instance IDs to file for reuse by later runs
        with open("instances_left_on.txt", "w") as instances_file:
            instances_file.write(f"{server_instance_id}\n")
            if client_instance_id != server_instance_id:
                instances_file.write(f"{client_instance_id}\n")

    print("Instance successfully stopped/terminated, goodbye")

    # 7- Delete the cleanup todo-list, because we already completed it.
    os.remove("instances_to_remove.txt")

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

    # 10- Print error message and exit with error if needed
    for cause, message in {
        "server_failure": "Failed to run WhistServer",
        "client_failure": "Failed to run WhistClient",
        "client_server_connection_failure": "Whist Client failed to connect to the server",
        "server_hang_detected": "WhistServer hang detected",
    }.items():
        if experiment_metadata[cause]:
            print(f"{message}! Exiting with error. Check the logs for more details.")
            return -1

    return 0
