#!/usr/bin/env python3

import os, sys, time, uuid
import subprocess
import pexpect

from helpers.common.timestamps_and_exit_tools import (
    exit_with_error,
    printgrey,
)
from helpers.common.pexpect_tools import wait_until_cmd_done
from helpers.common.constants import (
    ssh_connection_retries,
    aws_timeout_seconds,
    running_in_ci,
    username,
    free_lock_path,
    unique_lock_path,
    lock_ssh_timeout_seconds,
)

# Add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


def attempt_ssh_connection(ssh_command, log_file_handle, pexpect_prompt):
    """
    Attempt to establish a SSH connection to a remote machine. It is normal for the function to
    need several attempts before successfully opening a SSH connection to the remote machine.

    Args:
        ssh_command (str): The shell command to use to establish a SSH connection to the remote machine.
        log_file_handle (file): The file (already opened) to use for logging the terminal output from the
                         remote machine
        pexpect_prompt (file): The bash prompt printed by the shell on the remote machine when it is ready
                        to execute a command

    Returns:
        On success:
            pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process to be used from now on to interact
                        with the remote machine.
        On failure:
            None and exit with exitcode -1
    """
    for retries in range(ssh_connection_retries):
        child = pexpect.spawn(
            ssh_command, timeout=aws_timeout_seconds, logfile=log_file_handle.buffer
        )
        result_index = child.expect(
            [
                "Connection refused",
                "Are you sure you want to continue connecting (yes/no/[fingerprint])?",
                pexpect_prompt,
                pexpect.EOF,
                pexpect.TIMEOUT,
            ]
        )
        if result_index == 0:
            # If the connection was refused, sleep for 30s and then retry
            # (unless we exceeded the max number of retries)
            printgrey(
                f"\tSSH connection refused by host (retry {retries + 1}/{ssh_connection_retries})"
            )
            child.kill(0)
            time.sleep(30)
        elif result_index == 1 or result_index == 2:
            print(f"SSH connection established with EC2 instance!")
            # Confirm that we want to connect, if asked
            if result_index == 1:
                child.sendline("yes")
                child.expect(pexpect_prompt)
            # Wait for one more print of the prompt if required (ssh shell prompts are printed twice to stdout outside of CI)
            if not running_in_ci:
                child.expect(pexpect_prompt)
            # Return the pexpect_process handle to the caller
            return child
        elif result_index >= 3:
            # If the connection timed out, sleep for 30s and then retry
            # (unless we exceeded the max number of retries)
            printgrey(f"\tSSH connection timed out (retry {retries + 1}/{ssh_connection_retries})")
            child.kill(0)
            time.sleep(30)
    # Give up if the SSH connection was refused too many times.
    exit_with_error(
        f"SSH connection refused by host {ssh_connection_retries} times. Giving up now.",
        timestamps=None,
    )


def attempt_request_lock(instance_ip, ssh_key_path):
    """
    Request the E2E lock on the specified instance through SSH. We do that by moving the free lock
    (if it exists) to a unique path that is specific to the current run of the E2E. We use the `mv`
    operation, which is atomic in Linux.

    Args:
        instance_ip (str): The public IP of the instance for which we are trying to acquire the lock
        ssh_key_path (str): The path to the SSH key to be used to access the instance via SSH

    Returns:
        success (bool): indicates whether the locking succeeded.
    """
    subproc_handle = subprocess.Popen(
        f'ssh -i {ssh_key_path} -o ConnectTimeout={lock_ssh_timeout_seconds} -o StrictHostKeyChecking=no \
         {username}@{instance_ip} "mv {free_lock_path} {unique_lock_path}"',
        shell=True,
        stdout=subprocess.PIPE,
    )
    return_code = subproc_handle.poll()
    return return_code == 0


def attempt_release_lock(instance_ip, ssh_key_path):
    """
    Release the E2E lock on the specified instance through SSH. We do that by moving the lock
    from a unique path that is specific to the current run of the E2E, to the path of the free lock.
    We use the `mv` operation, which is atomic in Linux.

    Args:
        instance_ip (str): The public IP of the instance for which we are trying to release the lock
        ssh_key_path (str): The path to the SSH key to be used to access the instance via SSH

    Returns:
        success (bool): indicates whether the unlocking succeeded.
    """
    subproc_handle = subprocess.Popen(
        f'ssh -i {ssh_key_path} -o ConnectTimeout={lock_ssh_timeout_seconds} -o StrictHostKeyChecking=no \
        {username}@{instance_ip} "mv {unique_lock_path} {free_lock_path}"',
        shell=True,
        stdout=subprocess.PIPE,
    )
    return_code = subproc_handle.poll()
    return return_code == 0


def wait_for_apt_locks(pexpect_process, pexpect_prompt):
    """
    This function is used to prevent lock contention issues between E2E commands that use `apt` or `dpkg`
    and other Linux processes. These issues are especially common on a EC2 instance's first boot.

    We simply wait until no process holds the following locks (more could be added in the future):
    - /var/lib/dpkg/lock
    - /var/lib/apt/lists/lock
    - /var/cache/apt/archives/lock

    Args:
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process monitoring the execution of the process
                        on the remote machine
        pexpect_prompt (str): The bash prompt printed by the shell on the remote machine when it is ready to
                        execute a new command

    Returns:
        None
    """
    pexpect_process.sendline(
        "while sudo fuser /var/{lib/{dpkg,apt/lists},cache/apt/archives}/lock >/dev/null 2>&1; do echo 'Waiting for apt locks...'; sleep 1; done"
    )
    wait_until_cmd_done(pexpect_process, pexpect_prompt)


def reboot_instance(pexpect_process, ssh_cmd, log_file_handle, pexpect_prompt):
    """
    Reboot a remote machine and establish a new SSH connection after the machine comes back up.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn):  The Pexpect process created with pexpect.spawn(...) and to
                                                    be used to interact with the remote machine
        ssh_cmd (str):  The shell command to use to establish a new SSH connection to the remote machine after
                        the current connection is broken by the reboot.
        log_file_handle (file): The file (already opened) to use for logging the terminal output from the remote
                                machine
        pexpect_prompt (str):   The bash prompt printed by the shell on the remote machine when it is ready to
                                execute a command

    Returns:
        pexpect_process (pexpect.pty_spawn.spawn):  The new Pexpect process created with pexpect.spawn(...) and to
                                                    be used to interact with the remote machine after the reboot
    """
    # Trigger the reboot
    pexpect_process.sendline("sudo reboot")
    pexpect_process.kill(0)

    # Connect again
    pexpect_process = attempt_ssh_connection(ssh_cmd, log_file_handle, pexpect_prompt)
    print("Reboot complete")
    return pexpect_process
