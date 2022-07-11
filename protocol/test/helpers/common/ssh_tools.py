#!/usr/bin/env python3

import os, sys, time, uuid
import subprocess
import pexpect
import paramiko

from helpers.common.timestamps_and_exit_tools import (
    exit_with_error,
    printcolor,
)
from helpers.common.pexpect_tools import (
    wait_until_cmd_done,
    get_command_exit_code,
)
from helpers.common.constants import (
    ssh_connection_retries,
    aws_timeout_seconds,
    running_in_ci,
    username,
    free_lock_path,
    unique_lock_path,
    lock_ssh_timeout_seconds,
    path_to_all_locks,
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
            printcolor(
                f"\tSSH connection refused by host (retry {retries + 1}/{ssh_connection_retries})",
                "grey",
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
            printcolor(
                f"\tSSH connection timed out (retry {retries + 1}/{ssh_connection_retries})", "grey"
            )
            child.kill(0)
            time.sleep(30)
    # Give up if the SSH connection was refused too many times.
    exit_with_error(
        f"SSH connection refused by host {ssh_connection_retries} times. Giving up now.",
        timestamps=None,
    )


def run_single_ssh_command(instance_ip, ssh_key_path, timeout, command):
    """
    Execute a single command on a remote instance via SSH, and get the exit code. This function is less flexible
    but safer than pexpect

    Args:
        instance_ip (str): The public IP of the instance where we want to run the command
        ssh_key_path (str): The path to the SSH key to be used to access the instance via SSH
        timeout (int): The timeout in seconds for the command
        command (str): The command to run on the remote instance

    Returns:
        return_code (int): The exit code of the command (on the remote machine).
    """
    client = paramiko.SSHClient()
    client.load_system_host_keys()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    private_key = paramiko.RSAKey.from_private_key_file(ssh_key_path)
    try:
        client.connect(instance_ip, username=username, key_filename=ssh_key_path, timeout=timeout)
        _, stdout, stderr = client.exec_command(command)
        return_code = stdout.channel.recv_exit_status()
    except Exception as e:
        print("Caught SSH connect exception:")
        return -1
    # To enable printing the stdout/stderr, uncomment the lines below:
    # stdout = stdout.read().decode("utf-8") or ""
    # stderr = stderr.read().decode("utf-8") or ""
    # if len(stdout) > 0:
    #     print(stdout)
    # if len(stderr) > 0:
    #     print(stderr)
    return return_code


def attempt_request_lock(instance_ip, ssh_key_path, create_lock):
    """
    Request the E2E lock on the specified instance through SSH. We do that by moving the free lock
    (if it exists) to a unique path that is specific to the current run of the E2E. We use the `mv`
    operation, which is atomic in Linux.

    Args:
        instance_ip (str): The public IP of the instance for which we are trying to acquire the lock
        ssh_key_path (str): The path to the SSH key to be used to access the instance via SSH
        create_lock (book): Whether we need to create a lock for the fist time

    Returns:
        success (bool): indicates whether the locking succeeded.
    """
    creation_command = f"test -f {free_lock_path} || touch {free_lock_path}"
    import_lock_command = f"test -f {unique_lock_path}"
    locking_command = f"mv {free_lock_path} {unique_lock_path}"

    if create_lock:
        print(f"Creating lock on instance `{instance_ip}`...")
        # Create the lock if needed
        return_code = run_single_ssh_command(
            instance_ip, ssh_key_path, lock_ssh_timeout_seconds, creation_command
        )
        if return_code != 0:
            printcolor(f"Failed to create lock on instance `{instance_ip}`", "yellow")
            return False
        print(f"Successfully created lock on instance `{instance_ip}`!")

    # Attempt to import lock first
    return_code = run_single_ssh_command(
        instance_ip, ssh_key_path, lock_ssh_timeout_seconds, import_lock_command
    )
    if return_code == 0:
        print(
            f"Successfully imported lock {unique_lock_path} on instance `{instance_ip}` from previous session!"
        )
        return True

    # Attempt to grab the free lock
    return_code = run_single_ssh_command(
        instance_ip, ssh_key_path, lock_ssh_timeout_seconds, locking_command
    )
    print(f"Get lock return code: {return_code}")
    if return_code == 0:
        print(
            f"Successfully acquired the free lock on instance `{instance_ip}`! The lock is now named {unique_lock_path}!"
        )
        return True
    else:
        return False


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
    print(f"Attempting to release the lock on instance `{instance_ip}`...")

    # Check if we hold the lock. If we don't hold it, no further action is required.
    # We need to explicitly check because if we do hold the lock, and unlocking fails,
    # we need to keep retrying until we succeed in unlocking.
    check_lock_command = f"test -f {unique_lock_path}"
    return_code = run_single_ssh_command(
        instance_ip, ssh_key_path, lock_ssh_timeout_seconds, check_lock_command
    )
    if return_code == 1:
        print(
            f"We do not hold the lock on instance `{instance_ip}`. Either it was already released, or it was never acquired. No further action required."
        )
        return True

    # Unlock
    unlocking_command = f"mv {unique_lock_path} {free_lock_path}"
    return_code = run_single_ssh_command(
        instance_ip, ssh_key_path, lock_ssh_timeout_seconds, unlocking_command
    )

    if return_code == 0:
        print(
            f"Successfully released the lock {unique_lock_path} on instance `{instance_ip}`! The lock is now available at {free_lock_path}!"
        )
        return True
    else:
        print(
            f"Could not release lock {unique_lock_path} on instance `{instance_ip}`. Retry unlocking later..."
        )
        print()
        return False


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


def force_unlock(pexpect_process, pexpect_prompt):
    """
    This function is used to force unlock a machine in case a E2E agent crashed while holding the lock, and we
    do not know the lock's name.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process monitoring the execution of the process
                        on the remote machine
        pexpect_prompt (str): The bash prompt printed by the shell on the remote machine when it is ready to
                        execute a new command

    Returns:
        success (bool): whether we succeeded in releasing the lock
    """
    pexpect_process.sendline(f"rm -f {path_to_all_locks}; touch {free_lock_path}")
    wait_until_cmd_done(pexpect_process, pexpect_prompt)

    # Success is defined as: no LOCK-* files exist and the free_lock file exists
    pexpect_process.sendline(f"test -f {path_to_all_locks}")
    wait_until_cmd_done(pexpect_process, pexpect_prompt)
    delete_locks_success = get_command_exit_code(pexpect_process, pexpect_prompt) == 1

    pexpect_process.sendline(f"test -f {free_lock_path}")
    wait_until_cmd_done(pexpect_process, pexpect_prompt)
    create_free_lock_success = get_command_exit_code(pexpect_process, pexpect_prompt) == 0

    return delete_locks_success and create_free_lock_success
