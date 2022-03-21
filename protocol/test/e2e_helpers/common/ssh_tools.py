#!/usr/bin/env python3

import os, sys, time
import pexpect

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
    print(f"SSH connection refused by host {max_retries} times. Giving up now.")
    sys.exit(-1)


def wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci, return_output=False):
    """
    Wait until the currently-running command on a remote machine finishes its execution on the shell
    monitored to by a pexpect process.

    If the machine running this script is not a CI runner, handling color/formatted stdout on the remote
    machine will require special care. In particular, the shell prompt will be printed twice, and we have
    to wait for that to happen before sending another command.

    This function also has the option to return the shell stdout in a list of strings format.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process monitoring the execution of the process
                        on the remote machine
        pexpect_prompt (str): The bash prompt printed by the shell on the remote machine when it is ready to
                        execute a new command
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI
        return_output (bool): A boolean controlling whether to return the stdout output in a list of strings format

    Returns:
        On Success:
            pexpect_output (list): the stdout output of the command, with one entry for each line of the original
                        output. If return_output=False, pexpect_output is set to None
        On Failure:
            None and exit with exitcode -1
    """
    result = pexpect_process.expect([pexpect_prompt, pexpect.exceptions.TIMEOUT, pexpect.EOF])

    # Handle timeout and error cases
    if result == 1:
        print("Error: pexpect process timed out! Check the logs for troubleshooting.")
        sys.exit(-1)
    elif result == 2:
        print(
            "Error: pexpect process encountered an unexpected exception! Check the logs for troubleshooting."
        )
        sys.exit(-1)

    # Clean stdout output and save it in a list, one line per element. We need to do this before calling expect
    # again (if we are not in CI), otherwise the output will be overwritten.
    pexpect_output = None
    if return_output:
        pexpect_output = [
            line.replace("\r", "").replace("\n", "")
            for line in pexpect_process.before.decode("utf-8").strip().split("\n")
        ]

    # Colored/formatted stdout (such as the bash prompt) is duplicated when piped to a pexpect process.
    # This is likely a platform-dependent behavior and does not occur when running this script in CI. If
    # we are not in CI, we need to wait for the duplicated prompt to be printed before moving forward, otherwise
    # the duplicated prompt will interfere with the next command that we send.
    if not running_in_ci:
        pexpect_process.expect(pexpect_prompt)

    return pexpect_output


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


def apply_dpkg_locking_fixup(pexpect_process, pexpect_prompt, running_in_ci):
    """
    Prevent dpkg locking issues such as the following one:
    - E: Could not get lock /var/lib/dpkg/lock-frontend. It is held by process 2392 (apt-get)
    - E: Unable to acquire the dpkg frontend lock (/var/lib/dpkg/lock-frontend), is another process using it?

    Args:
        pexpect_process (pexpect.pty_spawn.spawn):  The Pexpect process created with pexpect.spawn(...) and to be used
                                                    to interact with the remote machine
        pexpect_prompt (str):   The bash prompt printed by the shell on the remote machine when
                                it is ready to execute a command
        running_in_ci (bool):   A boolean indicating whether this script is currently running in CI

    Returns:
        None
    """

    dpkg_commands = [
        "sudo kill -9 `sudo lsof /var/lib/dpkg/lock-frontend | awk '{print $2}' | tail -n 1`",
        "sudo kill -9 `sudo lsof /var/lib/apt/lists/lock | awk '{print $2}' | tail -n 1`",
        "sudo kill -9 `sudo lsof /var/lib/dpkg/lock | awk '{print $2}' | tail -n 1`",
        "sudo killall apt apt-get",
        "sudo pkill -9 apt",
        "sudo pkill -9 apt-get",
        "sudo pkill -9 dpkg",
        "sudo rm /var/lib/apt/lists/lock; \
        sudo rm /var/lib/apt/lists/lock-frontend; sudo rm /var/cache/apt/archives/lock; sudo rm /var/lib/dpkg/lock",
        "sudo dpkg --configure -a",
    ]
    for command in dpkg_commands:
        pexpect_process.sendline(command)
        wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)
