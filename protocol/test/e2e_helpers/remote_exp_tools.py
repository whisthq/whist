#!/usr/bin/env python3

import pexpect

# Get tools to run operations on a dev instance via SSH
from dev_instance_tools import (
    wait_until_cmd_done,
)


def extract_server_logs_from_instance(
    pexpect_process,
    pexpect_prompt,
    server_docker_id,
    ssh_key_path,
    username,
    server_hostname,
    timeout_value,
    perf_logs_folder_name,
    log_grabber_log,
    running_in_ci,
):
    """
    Extract the logs related to the run of the Whist server mandelbox (browsers/chrome) on a remote machine. This should be called after the client/server connection has terminated, but before the server container is stopped/destroyed.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process created with pexpect.spawn(...) and to be used to interact with the remote machine
        pexpect_prompt (str): The bash prompt printed by the shell on the remote machine when it is ready to execute a command
        server_docker_id (str): The Docker ID of the container running the Whist server (browsers/chrome mandelbox) on the remote machine
        ssh_key_path (str): The path (on the machine where this script is run) to the file storing the public RSA key used for SSH connections
        username (str): The username to be used on the remote machine (default is 'ubuntu')
        server_hostname(str): The host name of the remote machine where the server was running on
        timeout_value (int): The amount of time to wait before timing out the attemps to gain a SSH connection to the remote machine.
        perf_logs_folder_name (str): The path to the folder (on the machine where this script is run) where to store the logs
        log_grabber_log (file object): The file (already opened) to use for logging the terminal output from the shell process used to download the logs
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI

    Return:
        None

    """
    command = "rm -rf ~/perf_logs/server; mkdir -p ~/perf_logs/server"
    pexpect_process.sendline(command)
    wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)

    server_logfiles = [
        "/usr/share/whist/server.log",
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
        "/var/log/whist/protocol-out.log",
    ]
    for server_file_path in server_logfiles:
        command = f"docker cp {server_docker_id}:{server_file_path} ~/perf_logs/server/"
        pexpect_process.sendline(command)
        wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)

    # Download all the logs from the AWS machine
    command = f"scp -r -i {ssh_key_path} {username}@{server_hostname}:~/perf_logs/server {perf_logs_folder_name}"

    local_process = pexpect.spawn(command, timeout=timeout_value, logfile=log_grabber_log.buffer)
    local_process.expect(["\$", pexpect.EOF])
    local_process.kill(0)


def extract_client_logs_from_instance(
    pexpect_process,
    pexpect_prompt,
    client_docker_id,
    ssh_key_path,
    username,
    client_hostname,
    timeout_value,
    perf_logs_folder_name,
    log_grabber_log,
    running_in_ci,
):
    """
    Extract the logs related to the run of the Whist dev client (development/client) on a remote machine. This should be called after the client/server connection has terminated, but before the dev client container is stopped/destroyed.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process created with pexpect.spawn(...) and to be used to interact with the remote machine
        pexpect_prompt (str): The bash prompt printed by the shell on the remote machine when it is ready to execute a command
        client_docker_id (str): The Docker ID of the container running the Whist dev client (development/client mandelbox) on the remote machine
        ssh_key_path (str): The path (on the machine where this script is run) to the file storing the public RSA key used for SSH connections
        username (str): The username to be used on the remote machine (default is 'ubuntu')
        client_hostname(str): The host name of the remote machine where the dev client was running on
        timeout_value (int): The amount of time to wait before timing out the attemps to gain a SSH connection to the remote machine.
        perf_logs_folder_name (str): The path to the folder (on the machine where this script is run) where to store the logs
        log_grabber_log (file object): The file (already opened) to use for logging the terminal output from the shell process used to download the logs
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI

    Return:
        None

    """
    command = "rm -rf ~/perf_logs/client; mkdir -p ~/perf_logs/client"
    pexpect_process.sendline(command)
    wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)

    client_logfiles = [
        "/usr/share/whist/client.log",
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
        "/var/log/whist/protocol-out.log",
    ]
    for client_file_path in client_logfiles:
        command = f"docker cp {client_docker_id}:{client_file_path} ~/perf_logs/client/"
        pexpect_process.sendline(command)
        wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)

    # Download all the logs from the AWS machine
    command = f"scp -r -i {ssh_key_path} {username}@{client_hostname}:~/perf_logs/client {perf_logs_folder_name}"

    local_process = pexpect.spawn(command, timeout=timeout_value, logfile=log_grabber_log.buffer)
    local_process.expect(["\$", pexpect.EOF])
    local_process.kill(0)
