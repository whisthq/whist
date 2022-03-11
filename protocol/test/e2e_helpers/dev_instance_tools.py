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


def wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci, new_timeout=None):
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
        new_timeout (int): If a value is passed to new_timeout, we will use it to reset the pexpect process's timeout

    Returns:
        None
    """
    # Execute the command and apply the desired timeout if the new_timeout parameter is set.
    result = (
        pexpect_process.expect(
            [pexpect_prompt, pexpect.exceptions.TIMEOUT, pexpect.EOF], timeout=new_timeout
        )
        if new_timeout
        else pexpect_process.expect([pexpect_prompt, pexpect.exceptions.TIMEOUT, pexpect.EOF])
    )
    # Handle timeout and error cases
    if result == 1:
        print("Error: pexpect process timed out! Check the logs for troubleshooting.")
        sys.exit(-1)
    elif result == 2:
        print(
            "Error: pexpect process encountered an unexpected exception! Check the logs for troubleshooting."
        )
        sys.exit(-1)

    # On a SSH connection, the prompt is printed two times on Mac (because of some obscure reason related to encoding and/or color printing on terminal)
    if not running_in_ci:
        pexpect_process.expect(pexpect_prompt)


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


def configure_aws_credentials(
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
        On success:
            0
        On failure:
            -1 on fatal error, -2 on error that will likely be gone if we retry
    """

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
            return -1
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
        return -1

    pexpect_process.sendline("sudo apt-get -y update")
    wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)

    # Check if the AWS CLI is installed, and install it if not.
    pexpect_process.sendline("which aws")

    try:
        pexpect_process.expect(r"/usr(.*?)aws", timeout=5)
        print("AWS-CLI is already installed")
    except pexpect.exceptions.TIMEOUT:
        # Download and install AWS CLI manually to avoid frequent apt install outages
        print("Installing AWS-CLI manually")
        # https://docs.aws.amazon.com/cli/latest/userguide/getting-started-install.html
        install_commands = [
            # Download AWS installer
            "curl https://awscli.amazonaws.com/awscli-exe-linux-x86_64.zip -o awscliv2.zip",
            # Download the unzip program
            "sudo apt-get install -y unzip",
            # Unzip the AWS installer
            "unzip -o awscliv2.zip",
            # Remove the zip file
            "rm awscliv2.zip",
            # Install AWS
            "sudo ./aws/install",
        ]

        for command in install_commands:
            pexpect_process.sendline(command)
            # We need to restore the timeout, which we changed to 5s above
            wait_until_cmd_done(
                pexpect_process, pexpect_prompt, running_in_ci, new_timeout=aws_timeout_seconds
            )

        print("AWS-CLI installed manually")

    # Configure AWS using the list below of tuples of the form:
    # (<command>, <stdout pattern to wait for before proceeding to the next command>)
    configuration_commands = [
        ("aws configure", "AWS Access Key ID"),
        (aws_access_key_id, "AWS Secret Access Key"),
        (aws_secret_access_key, "Default region name"),
        ("", "Default output format"),
        ("", pexpect_prompt),
    ]

    for command, prompt in configuration_commands:
        pexpect_process.sendline(command)
        # Passing running_in_ci=True if the prompt is not the pexpect_prompt,
        # because only the pexpect_prompt can be printed twice by the process
        # due to the encoding of colors (see wait_until_cmd_done docs)
        running_in_ci_setting = (prompt != pexpect_prompt) or running_in_ci
        wait_until_cmd_done(pexpect_process, prompt, running_in_ci=running_in_ci_setting)

    print("AWS configuration is now complete!")
    return 0


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
