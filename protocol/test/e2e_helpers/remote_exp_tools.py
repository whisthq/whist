#!/usr/bin/env python3

import pexpect

# Get tools to run operations on a dev instance via SSH
from dev_instance_tools import (
    wait_until_cmd_done,
)


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
    Extract the logs related to the run of the Whist server/client mandelbox (browsers/chrome) on a remote machine. This should be called after the client->server connection has terminated, but before the server/client container is stopped/destroyed.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process created with pexpect.spawn(...) and to be used to interact with the remote machine
        pexpect_prompt (str): The bash prompt printed by the shell on the remote machine when it is ready to execute a command
        docker_id (str): The Docker ID of the container running the Whist server/client (browsers/chrome or development/client mandelbox) on the remote machine
        ssh_key_path (str): The path (on the machine where this script is run) to the file storing the public RSA key used for SSH connections
        username (str): The username to be used on the remote machine (default is 'ubuntu')
        hostname(str): The host name of the remote machine where the server/client was running on
        timeout_value (int): The amount of time to wait before timing out the attemps to gain a SSH connection to the remote machine.
        perf_logs_folder_name (str): The path to the folder (on the machine where this script is run) where to store the logs
        log_grabber_log (file object): The file (already opened) to use for logging the terminal output from the shell process used to download the logs
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI

    Return:
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
        "/var/log/whist/protocol-out.log",
    ]
    for file_path in logfiles:
        command = f"docker cp {docker_id}:{file_path} ~/perf_logs/{role}/"
        pexpect_process.sendline(command)
        wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)

    # Download all the logs from the AWS machine
    command = (
        f"scp -r -i {ssh_key_path} {username}@{hostname}:~/perf_logs/{role} {perf_logs_folder_name}"
    )

    local_process = pexpect.spawn(command, timeout=timeout_value, logfile=log_grabber_log.buffer)
    local_process.expect(["\$", pexpect.EOF])
    local_process.kill(0)
