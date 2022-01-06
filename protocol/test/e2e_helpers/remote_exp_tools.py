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
    aws_timeout,
    perf_logs_folder_name,
    log_grabber_log,
):
    command = "rm -rf ~/perf_logs/server; mkdir -p ~/perf_logs/server"
    pexpect_process.sendline(command)
    wait_until_cmd_done(pexpect_process, pexpect_prompt)

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
        command = "docker cp {}:{} ~/perf_logs/server/".format(server_docker_id, server_file_path)
        pexpect_process.sendline(command)
        wait_until_cmd_done(pexpect_process, pexpect_prompt)

    # Download all the logs from the AWS machine
    command = "scp -r -i {} {}@{}:~/perf_logs/server {}".format(
        ssh_key_path, username, server_hostname, perf_logs_folder_name
    )

    local_process = pexpect.spawn(command, timeout=aws_timeout, logfile=log_grabber_log.buffer)
    local_process.expect(["\$", pexpect.EOF])
    local_process.kill(0)


def extract_client_logs_from_instance(
    pexpect_process,
    pexpect_prompt,
    client_docker_id,
    ssh_key_path,
    username,
    client_hostname,
    aws_timeout,
    perf_logs_folder_name,
    log_grabber_log,
):
    command = "rm -rf ~/perf_logs/client; mkdir -p ~/perf_logs/client"
    pexpect_process.sendline(command)
    wait_until_cmd_done(pexpect_process, pexpect_prompt)

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
        command = "docker cp {}:{} ~/perf_logs/client/".format(client_docker_id, client_file_path)
        pexpect_process.sendline(command)
        wait_until_cmd_done(pexpect_process, pexpect_prompt)

    # Download all the logs from the AWS machine
    command = "scp -r -i {} {}@{}:~/perf_logs/client {}".format(
        ssh_key_path, username, client_hostname, perf_logs_folder_name
    )

    local_process = pexpect.spawn(command, timeout=aws_timeout, logfile=log_grabber_log.buffer)
    local_process.expect(["\$", pexpect.EOF])
    local_process.kill(0)