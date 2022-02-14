#!/usr/bin/env python3

import pexpect
import os
import time
import subprocess
import platform


def get_whist_branch_name(running_in_ci):
    """
    Retrieve the branch name of the repository to which the folder from which this script is run belongs to.
    Args:
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI
    Returns:
        On success:
            branch_name (string): The name of the branch
        On failure:
            empty string
    """

    branch_name = ""

    if running_in_ci:
        # In CI, the PR branch name is saved in GITHUB_REF_NAME, or in the GITHUB_HEAD_REF environment variable (in case this script is being run as part of a PR)
        b = os.getenv("GITHUB_REF_NAME").split("/")
        if len(b) != 2 or not b[0].isnumeric() or b[1] != "merge":
            branch_name = os.getenv("GITHUB_REF_NAME")
        else:
            branch_name = os.getenv("GITHUB_HEAD_REF")
    else:
        # Locally, we can find the branch using the 'git branch' command.
        # WARNING: this command will fail on detached HEADS.

        subproc_handle = subprocess.Popen("git branch", shell=True, stdout=subprocess.PIPE)
        subprocess_stdout = subproc_handle.stdout.readlines()

        for line in subprocess_stdout:
            converted_line = line.decode("utf-8").strip()
            if "*" in converted_line:
                branch_name = converted_line[2:]
                break

    return branch_name


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
            print("\tSSH connection refused by host (retry {}/{})".format(retries, max_retries))
            child.kill(0)
            time.sleep(10)
        elif result_index == 1 or result_index == 2:
            if result_index == 1:
                child.sendline("yes")
                child.expect(pexpect_prompt)
            print(f"SSH connection established with EC2 instance!")
            return child
        elif result_index >= 3:
            print("\tSSH connection timed out (retry {}/{})".format(retries, max_retries))
            child.kill(0)
            time.sleep(10)
    print("SSH connection refused by host {} times. Giving up now.".format(max_retries))
    exit()


def wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci):
    """
    Wait until the currently-running command on a remote machine finishes its execution on the shell monitored to by a pexpect process.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process monitoring the execution of the process on the remote machine
        pexpect_prompt (str): The bash prompt printed by the shell on the remote machine when it is ready to execute a new command
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI

    Returns:
        None
    """
    pexpect_process.expect(pexpect_prompt)
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
    running_in_ci,
    aws_credentials_filepath="~/.aws/credentials",
):
    """
    Configure AWS credentials on a remote machine by copying them from the ones configures on the machine where this script is being run.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process created with pexpect.spawn(...) and to be used to interact with the remote machine
        pexpect_prompt (str): The bash prompt printed by the shell on the remote machine when it is ready to execute a command
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
        # In CI, the aws credentials are stored in env variables instead of a file on disk
        aws_access_key_id = os.getenv("AWS_ACCESS_KEY_ID")
        aws_secret_access_key = os.getenv("AWS_SECRET_ACCESS_KEY")
        if aws_access_key_id == None or aws_secret_access_key == None:
            return -1
    else:
        aws_credentials_filepath_expanded = os.path.expanduser(aws_credentials_filepath)

        if not os.path.isfile(aws_credentials_filepath_expanded):
            print(
                "Could not find local AWS credential file at path {}!".format(
                    aws_credentials_filepath
                )
            )
            return -1
        aws_credentials_file = open(aws_credentials_filepath_expanded, "r")
        for line in aws_credentials_file.readlines():
            if "aws_access_key_id" in line:
                aws_access_key_id = line.strip().split()[2]
            elif "aws_secret_access_key" in line:
                aws_secret_access_key = line.strip().split()[2]
                break
        aws_credentials_file.close()
        if aws_access_key_id == "" or aws_secret_access_key == "":
            print(
                "Could not parse AWS credentials from file at path {}!".format(
                    aws_credentials_filepath
                )
            )
            return -1

    pexpect_process.sendline("sudo apt-get update")
    result = pexpect_process.expect(["Do you want to continue?", pexpect_prompt])
    if result == 0:
        pexpect_process.sendline("Y")
        wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)
    elif not running_in_ci:
        pexpect_process.expect(pexpect_prompt)

    pexpect_process.sendline("which aws")
    try:
        pexpect_process.expect(r"/usr(.*?)aws", timeout=5)
        print("AWS-CLI is already installed")
    except pexpect.exceptions.TIMEOUT:
        print("Installing AWS-CLI manually")
        # Download and install AWS cli manually to avoid frequent apt install outages
        # https://docs.aws.amazon.com/cli/latest/userguide/getting-started-install.html
        pexpect_process.sendline(
            "curl https://awscli.amazonaws.com/awscli-exe-linux-x86_64.zip -o awscliv2.zip"
        )
        wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)
        pexpect_process.sendline("sudo apt install -y unzip")
        wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)
        pexpect_process.sendline("unzip -o awscliv2.zip")
        wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)
        pexpect_process.sendline("rm awscliv2.zip")
        wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)
        pexpect_process.sendline("sudo ./aws/install")
        wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)
        print("AWS-CLI installed")

    pexpect_process.sendline("aws configure")
    pexpect_process.expect("AWS Access Key ID")
    pexpect_process.sendline(aws_access_key_id)
    pexpect_process.expect("AWS Secret Access Key")
    pexpect_process.sendline(aws_secret_access_key)
    pexpect_process.expect("Default region name")
    pexpect_process.sendline("")
    pexpect_process.expect("Default output format")
    pexpect_process.sendline("")
    wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)

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

    print(
        "Cloning branch {} of the whisthq/whist repository on the AWS instance ...".format(
            branch_name
        )
    )

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
