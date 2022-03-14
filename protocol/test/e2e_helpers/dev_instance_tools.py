#!/usr/bin/env python3

import pexpect
import os
import sys
import time
import subprocess
import platform

from e2e_helpers.local_tools import (
    get_whist_branch_name,
)


def attempt_ssh_connection(
    ssh_command, timeout_value, log_file_handle, pexpect_prompt, max_retries
):
    """
    Attempt to establish a SSH connection to a remote machine. It is normal for the function to need several attempts before successfully opening a SSH connection to the remote machine.

    Args:
        ssh_cmd (str): The shell command to use to establish a SSH connection to the remote machine.
        timeout_value (int): The amount of time to wait before timing out the attemps to gain a SSH connection to the remote machine.
        log_file_handle (file object): The file (already opened) to use for logging the terminal output from the remote machine
        pexpect_prompt (str): The bash prompt printed by the shell on the remote machine when it is ready to execute a command
        max_retries (int): The maximum number of attempts to use before giving up on establishing a SSH connection to the remote machine

    Returns:
        On success:
            pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process to be used from now on to interact with the remote machine.
        On failure:
            None
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
            print(f"\tSSH connection refused by host (retry {retries + 1}/{max_retries})")
            child.kill(0)
            time.sleep(30)
        elif result_index == 1 or result_index == 2:
            if result_index == 1:
                child.sendline("yes")
                child.expect(pexpect_prompt)
            print(f"SSH connection established with EC2 instance!")
            return child
        elif result_index >= 3:
            print(f"\tSSH connection timed out (retry {retries + 1}/{max_retries})")
            child.kill(0)
            time.sleep(30)
    print(f"SSH connection refused by host {max_retries} times. Giving up now.")
    sys.exit(-1)


def wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci, return_output=False):
    """
    Wait until the currently-running command on a remote machine finishes its execution on the shell monitored to by a pexpect process.

    N.B: Whenever running_in_ci=False, it is unsafe to parse the pexpect process's stdout output (with pexpect_process.before) after
        a call to wait_until_cmd_done. If this script is running outside of a CI environment (running_in_ci=False), all bash prompts
        are likely to be printed two times. If we are looking to parse the stdout output, we need the stdout that comes before the
        first prompt is printed. Getting the stdout after a call wait_until_cmd_done, however, will discard the output of the command
        executed by the pexpect_process and only return the stdout (usually empty) between the first and the second prompt.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process monitoring the execution of the process on the remote machine
        pexpect_prompt (str): The bash prompt printed by the shell on the remote machine when it is ready to execute a new command
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI
        watched_stdout_pattern (str): A string that we are looking for in the output of the command sent to the pexpect process

    Returns:
        On Success:
            If return_output is True:
                stdout_list (list): the stdout output of the command, with one entry for each line of the original output.
            Otherwise:
                None
        On Failure:
            None because the function will exit with error.

    """
    result = pexpect_process.expect([pexpect_prompt, pexpect.exceptions.TIMEOUT, pexpect.EOF])

    watched_stdout_pattern_found = False

    # Handle timeout and error cases
    if result == 1:
        print("Error: pexpect process timed out! Check the logs for troubleshooting.")
        sys.exit(-1)
    elif result == 2:
        print(
            "Error: pexpect process encountered an unexpected exception! Check the logs for troubleshooting."
        )
        sys.exit(-1)

    # Check for watched stdout pattern
    pexpect_output = None
    if return_output:
        pexpect_output = [
            line.replace("\r", "").replace("\n", "")
            for line in pexpect_process.before.decode("utf-8").strip().split("\n")
        ]

    # On a SSH connection, the prompt is printed two times on Mac (because of some obscure reason related to encoding and/or color printing on terminal)
    if not running_in_ci:
        pexpect_process.expect(pexpect_prompt)

    return pexpect_output


def reboot_instance(
    pexpect_process, ssh_cmd, timeout_value, log_file_handle, pexpect_prompt, retries, running_in_ci
):
    """
    Reboot a remote machine and establish a new SSH connection after the machine comes back up.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process created with pexpect.spawn(...) and to be used to interact with the remote machine
        ssh_cmd (str): The shell command to use to establish a new SSH connection to the remote machine after the current connection is broken by the reboot.
        timeout_value (int): The amount of time to wait before timing out the attemps to gain a SSH connection to the remote machine.
        log_file_handle (file object): The file (already opened) to use for logging the terminal output from the remote machine
        pexpect_prompt (str): The bash prompt printed by the shell on the remote machine when it is ready to execute a command
        retries (int): Maximum number of attempts before giving up on gaining a new SSH connection after rebooting the remote machine.
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI
    Returns:
        None
    """

    pexpect_process.sendline(" ")
    wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)
    pexpect_process.sendline("sudo reboot")
    pexpect_process.kill(0)
    time.sleep(5)
    pexpect_process = attempt_ssh_connection(
        ssh_cmd, timeout_value, log_file_handle, pexpect_prompt, retries
    )
    print("Reboot complete")
    return pexpect_process


def apply_dpkg_locking_fixup(pexpect_process, pexpect_prompt, running_in_ci):
    """
    Prevent dpkg locking issues such as the following one:
    - E: Could not get lock /var/lib/dpkg/lock-frontend. It is held by process 2392 (apt-get)
    - E: Unable to acquire the dpkg frontend lock (/var/lib/dpkg/lock-frontend), is another process using it?
    - See also here: https://github.com/ray-project/ray/blob/master/doc/examples/lm/lm-cluster.yaml

    Args:
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process created with pexpect.spawn(...) and to be used to interact with the remote machine
        pexpect_prompt (str): The bash prompt printed by the shell on the remote machine when it is ready to execute a command
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI
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
        "sudo rm /var/lib/apt/lists/lock; sudo rm /var/lib/apt/lists/lock-frontend; sudo rm /var/cache/apt/archives/lock; sudo rm /var/lib/dpkg/lock",
        "sudo dpkg --configure -a",
    ]
    for command in dpkg_commands:
        pexpect_process.sendline(command)
        wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)


def install_and_configure_aws(
    pexpect_process,
    pexpect_prompt,
    aws_timeout_seconds,
    running_in_ci,
    aws_credentials_filepath=os.path.join(os.path.expanduser("~"), ".aws", "credentials"),
):
    """
    Configure AWS credentials on a remote machine by copying them from the ones configures on the machine where this script is being run.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process created with pexpect.spawn(...) and to be used to interact with the remote machine
        pexpect_prompt (str): The bash prompt printed by the shell on the remote machine when it is ready to execute a command
        aws_timeout_seconds (int): Timeout to be used for the Pexpect process.
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI
        aws_credentials_filepath(str): The path to the file where AWS stores the credentials on the machine where this script is run
    Returns:
        None
    """

    # Step 1: Obtain AWS credentials

    aws_access_key_id = ""
    aws_secret_access_key = ""

    if running_in_ci:
        print("Getting the AWS credentials from environment variables...")
        # In CI, the aws credentials are stored in the following env variables
        aws_access_key_id = os.getenv("AWS_ACCESS_KEY_ID")
        aws_secret_access_key = os.getenv("AWS_SECRET_ACCESS_KEY")
    else:
        # Extract AWS credentials from aws configuration file on disk
        aws_credentials_filepath_expanded = os.path.expanduser(aws_credentials_filepath)
        if not os.path.isfile(aws_credentials_filepath_expanded):
            print(f"Could not find local AWS credential file at path {aws_credentials_filepath}!")
            sys.exit(-1)
        aws_credentials_file = open(aws_credentials_filepath_expanded, "r")
        for line in aws_credentials_file.readlines():
            if "aws_access_key_id" in line:
                aws_access_key_id = line.strip().split()[2]
            elif "aws_secret_access_key" in line:
                aws_secret_access_key = line.strip().split()[2]
                break
        aws_credentials_file.close()

    if (
        aws_access_key_id == None
        or aws_secret_access_key == None
        or len(aws_access_key_id) == 0
        or len(aws_secret_access_key) == 0
    ):
        print(f"Could not obtain the AWS credentials!")
        return False

    # Step 2: Install the AWS CLI if it's not already there

    pexpect_process.sendline("sudo apt-get -y update")
    wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)

    # Check if the AWS CLI is installed, and install it if not.
    pexpect_process.sendline("aws -v")
    stdout = wait_until_cmd_done(
        pexpect_process,
        pexpect_prompt,
        running_in_ci,
        return_output=True,
    )

    # Check if the message below, indicating that aws is not installed, is present in the output.
    error_msg = "Command 'aws' not found, but can be installed with:"
    aws_not_installed = any(error_msg in item for item in stdout if isinstance(item, str))

    # Attempt installation using apt-get

    if aws_not_installed:
        # Attemp to install using apt-get
        print("Installing AWS-CLI using apt-get")

        pexpect_process.sendline("sudo apt-get install -y awscli")
        stdout = wait_until_cmd_done(
            pexpect_process,
            pexpect_prompt,
            running_in_ci,
            return_output=True,
        )

        # apt-get fails from time to time
        error_msg = "E: Package 'awscli' has no installation candidate"
        apt_get_awscli_failed = any(error_msg in item for item in stdout if isinstance(item, str))

        if apt_get_awscli_failed:
            print(
                "Installing AWS-CLI using apt-get failed. This usually happens when the Ubuntu package lists are being updated."
            )

            # Attempt to install manually. This also fails from time to time, because we need apt-get to install the `unzip` package
            # https://docs.aws.amazon.com/cli/latest/userguide/getting-started-install.html
            print("Installing AWS-CLI from source")

            # Download the unzip program
            command = "sudo apt-get install -y unzip"
            pexpect_process.sendline(command)
            stdout = wait_until_cmd_done(
                pexpect_process, pexpect_prompt, running_in_ci, return_output=True
            )
            error_msg = "E: Package 'unzip' has no installation candidate"
            apt_get_unzip_failed = any(
                error_msg in item for item in stdout if isinstance(item, str)
            )

            if apt_get_unzip_failed:
                print(
                    "Installing 'unzip' using apt-get failed. This usually happens when the Ubuntu package lists are being updated."
                )
                print("Installing AWS-CLI from source failed")
                return False

            install_commands = [
                # Download AWS installer
                "curl https://awscli.amazonaws.com/awscli-exe-linux-x86_64.zip -o awscliv2.zip",
                # Unzip the AWS installer
                "unzip -o awscliv2.zip",
                # Remove the zip file
                "rm awscliv2.zip",
                # Install AWS
                "sudo ./aws/install",
            ]

            for command in install_commands:
                pexpect_process.sendline(command)
                wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)
            print("AWS-CLI installed manually")
    else:
        print("AWS-CLI is already installed")

    # Step 3: Set the AWS credentials
    access_key_cmd = f"aws configure set aws_access_key_id {aws_access_key_id}"
    pexpect_process.sendline(access_key_cmd)
    wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)
    secret_access_key_cmd = f"aws configure set aws_secret_access_key {aws_secret_access_key}"
    pexpect_process.sendline(secret_access_key_cmd)
    wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)

    print("AWS configuration is now complete!")
    return True


def clone_whist_repository_on_instance(
    github_token, pexpect_process, pexpect_prompt, running_in_ci
):
    """
    Clone the Whist repository on a remote machine, and check out the same branch used locally on the machine where this script is run.

    Args:
        github_token (str): The secret token to use to access the Whist private repository on GitHub
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process created with pexpect.spawn(...) and to be used to interact with the remote machine
        pexpect_prompt (str): The bash prompt printed by the shell on the remote machine when it is ready to execute a command
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI
    Returns:
        None
    """
    branch_name = get_whist_branch_name(running_in_ci)

    print(f"Cloning branch {branch_name} of the whisthq/whist repository on the AWS instance ...")

    # Retrieve whisthq/whist monorepo on the instance
    command = (
        "rm -rf whist; git clone -b "
        + branch_name
        + " https://"
        + github_token
        + "@github.com/whisthq/whist.git | tee ~/github_log.log"
    )

    pexpect_process.sendline(command)
    wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)

    print("Finished downloading whisthq/whist on EC2 instance")
