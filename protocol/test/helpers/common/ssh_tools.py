#!/usr/bin/env python3

import os, sys, time
import pexpect

from helpers.common.timestamps_and_exit_tools import (
    exit_with_error,
)
from helpers.common.pexpect_tools import wait_until_cmd_done

# Add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


def attempt_ssh_connection(
    ssh_command, timeout_value, log_file_handle, pexpect_prompt, max_retries, running_in_ci
):
    """
    Attempt to establish a SSH connection to a remote machine. It is normal for the function to
    need several attempts before successfully opening a SSH connection to the remote machine.

    Args:
        ssh_command (str): The shell command to use to establish a SSH connection to the remote machine.
        timeout_value (int): The amount of time to wait before timing out the attemps to gain a SSH connection
                       to the remote machine.
        log_file_handle (file): The file (already opened) to use for logging the terminal output from the
                         remote machine
        pexpect_prompt (file): The bash prompt printed by the shell on the remote machine when it is ready
                        to execute a command
        max_retries (int): The maximum number of attempts to use before giving up on establishing a SSH
                     connection to the remote machine

    Returns:
        On success:
            pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process to be used from now on to interact
                        with the remote machine.
        On failure:
            None and exit with exitcode -1
    """
    for retries in range(max_retries):
        child = pexpect.spawn(ssh_command, timeout=timeout_value, logfile=log_file_handle.buffer)
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
            print(f"\tSSH connection refused by host (retry {retries + 1}/{max_retries})")
            child.kill(0)
            time.sleep(30)
        elif result_index == 1 or result_index == 2:
            print(f"SSH connection established with EC2 instance!")
            # Confirm that we want to connect, if asked
            if result_index == 1:
                child.sendline("yes")
                child.expect(pexpect_prompt)
            # Wait for one more print of the prompt if required
            if not running_in_ci:
                child.expect(pexpect_prompt)
            # Return the pexpect_process handle to the caller
            return child
        elif result_index >= 3:
            # If the connection timed out, sleep for 30s and then retry
            # (unless we exceeded the max number of retries)
            print(f"\tSSH connection timed out (retry {retries + 1}/{max_retries})")
            child.kill(0)
            time.sleep(30)
    # Give up if the SSH connection was refused too many times.
    exit_with_error(
        f"SSH connection refused by host {max_retries} times. Giving up now.", timestamps=None
    )


def wait_for_apt_locks(pexpect_process, pexpect_prompt, running_in_ci):
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
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI

    Returns:
        None
    """
    pexpect_process.sendline(
        "while sudo fuser /var/{lib/{dpkg,apt/lists},cache/apt/archives}/lock >/dev/null 2>&1; do echo 'Waiting for apt locks...'; sleep 1; done"
    )
    wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)


def reboot_instance(
    pexpect_process, ssh_cmd, timeout_value, log_file_handle, pexpect_prompt, retries, running_in_ci
):
    """
    Reboot a remote machine and establish a new SSH connection after the machine comes back up.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn):  The Pexpect process created with pexpect.spawn(...) and to
                                                    be used to interact with the remote machine
        ssh_cmd (str):  The shell command to use to establish a new SSH connection to the remote machine after
                        the current connection is broken by the reboot.
        timeout_value (int):    The amount of time to wait before timing out the attemps to gain a SSH connection
                                to the remote machine.
        log_file_handle (file): The file (already opened) to use for logging the terminal output from the remote
                                machine
        pexpect_prompt (str):   The bash prompt printed by the shell on the remote machine when it is ready to
                                execute a command
        retries (int):  Maximum number of attempts before giving up on gaining a new SSH connection after
                        rebooting the remote machine.
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI

    Returns:
        pexpect_process (pexpect.pty_spawn.spawn):  The new Pexpect process created with pexpect.spawn(...) and to
                                                    be used to interact with the remote machine after the reboot
    """
    # Trigger the reboot
    pexpect_process.sendline("sudo reboot")
    pexpect_process.kill(0)

    # Connect again
    pexpect_process = attempt_ssh_connection(
        ssh_cmd, timeout_value, log_file_handle, pexpect_prompt, retries, running_in_ci
    )
    print("Reboot complete")
    return pexpect_process
