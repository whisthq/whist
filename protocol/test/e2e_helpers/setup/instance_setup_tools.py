#!/usr/bin/env python3

import pexpect
import os
import sys

from e2e_helpers.common.git_tools import (
    get_whist_branch_name,
)

from e2e_helpers.common.timestamps_and_exit_tools import (
    exit_with_error,
    printformat,
)

from e2e_helpers.common.constants import (
    SETUP_MAX_RETRIES,
    HOST_SETUP_TIMEOUT_SECONDS,
    TIMEOUT_EXIT_CODE,
    TIMEOUT_KILL_EXIT_CODE,
    aws_credentials_filepath,
    running_in_ci,
)

# Add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


def wait_for_apt_locks(remote_executor):
    """
    This function is used to prevent lock contention issues between E2E commands that use `apt` or `dpkg`
    and other Linux processes. These issues are especially common on a EC2 instance's first boot.

    We simply wait until no process holds the following locks (more could be added in the future):
    - /var/lib/dpkg/lock
    - /var/lib/apt/lists/lock
    - /var/cache/apt/archives/lock

    Args:
        remote_executor (RemoteExecutor):  The executor object to be used to run commands on the remote instance

    Returns:
        None
    """
    remote_executor.run_command(
        "while sudo fuser /var/{lib/{dpkg,apt/lists},cache/apt/archives}/lock >/dev/null 2>&1; do echo 'Waiting for apt locks...'; sleep 1; done"
    )


def prepare_instance_for_host_setup(remote_executor):
    # Wait for dpkg / apt locks
    wait_for_apt_locks(remote_executor)

    # Set dkpg frontend as non-interactive to avoid irrelevant warnings
    remote_executor.run_command(
        "echo 'debconf debconf/frontend select Noninteractive' | sudo debconf-set-selections"
    )

    # Wait again for dpkg / apt locks
    wait_for_apt_locks(remote_executor)

    # Clean, upgrade and update all the apt lists
    remote_executor.run_command(
        "sudo apt-get clean -y && sudo apt-get upgrade -y && sudo apt-get update -y"
    )


def get_aws_credentials():
    """
    Obtain the AWS credentials that are currently in use on the machine where this script is being run.
    Args:
        None
    Returns:
        aws_access_key_id (str): The AWS access key ID
        aws_secret_access_key (str): The AWS secret access key
    """
    # Step 1: Obtain AWS credentials from the local machine
    aws_access_key_id = ""
    aws_secret_access_key = ""
    if running_in_ci:
        # In CI, the AWS credentials are stored in the following env variables
        print("Getting the AWS credentials from environment variables...")
        aws_access_key_id = os.getenv("AWS_ACCESS_KEY_ID")
        aws_secret_access_key = os.getenv("AWS_SECRET_ACCESS_KEY")
    else:
        # Extract AWS credentials from aws configuration file on disk
        if not os.path.isfile(aws_credentials_filepath):
            exit_with_error(
                f"Could not find local AWS credential file at path {aws_credentials_filepath}!"
            )
        aws_credentials_file = open(aws_credentials_filepath, "r")

        # Read the AWS configuration file
        for line in aws_credentials_file.readlines():
            if "aws_access_key_id" in line:
                aws_access_key_id = line.strip().split()[2]
            elif "aws_secret_access_key" in line:
                aws_secret_access_key = line.strip().split()[2]
                break
        aws_credentials_file.close()

    # Sanity check
    if len(aws_access_key_id or "") == 0 or len(aws_secret_access_key or "") == 0:
        exit_with_error(f"Could not obtain the AWS credentials!")

    return aws_access_key_id, aws_secret_access_key


def install_and_configure_aws(remote_executor):
    """
    Install the AWS CLI and configure the AWS credentials on a remote machine by copying them
    from the ones configured on the machine where this script is being run.

    Args:
        remote_executor (RemoteExecutor):  The executor object to be used to run commands on the remote instance

    Returns:
        True if the AWS installation and configuration succeeded, False otherwise.
    """
    # Step 1: Obtain AWS credentials from the local machine
    aws_access_key_id, aws_secret_access_key = get_aws_credentials()

    # Wait for apt locks
    wait_for_apt_locks(remote_executor)

    # Step 2: Install the AWS CLI if it's not already there
    remote_executor.run_command("sudo apt-get -y update")
    # Check if the AWS CLI is installed, and install it if not.
    remote_executor.run_command("aws -v", ignore_exit_codes=True)

    # Check if the message below, indicating that aws is not installed, is present in the output.
    error_msg = "Command 'aws' not found, but can be installed with:"
    # Attempt installation using apt-get
    if remote_executor.expression_in_pexpect_output(error_msg):
        # Wait for apt locks
        wait_for_apt_locks(remote_executor)

        # Check if the apt-get installation failed (it happens from time to time)
        error_msg = "E: Package 'awscli' has no installation candidate"
        error_usr_msg = "Installing AWS-CLI using apt-get failed. This usually happens when the Ubuntu package lists are being updated."
        description = "Installing AWS-CLI using apt-get"
        remote_executor.run_command(
            "sudo apt-get install -y awscli",
            description,
            errors_to_handle=[(error_msg, error_usr_msg, None)],
        )

    else:
        print("AWS CLI is already installed")

    # Step 3: Set the AWS credentials
    access_key_cmd = f"aws configure set aws_access_key_id {aws_access_key_id}"
    remote_executor.run_command(access_key_cmd)
    secret_access_key_cmd = f"aws configure set aws_secret_access_key {aws_secret_access_key}"
    remote_executor.run_command(secret_access_key_cmd)

    print("AWS configuration is now complete!")


def clone_whist_repository(github_token, remote_executor):
    """
    Clone the Whist repository on a remote machine, and check out the same branch used locally
    on the machine where this script is run.

    Args:
        github_token:   The secret token to use to access the Whist private repository on GitHub
        remote_executor (RemoteExecutor):  The executor object to be used to run commands on the remote instance

    Returns:
        None
    """
    branch_name = get_whist_branch_name()

    # Retrieve whisthq/whist monorepo on the instance
    command = (
        "rm -rf whist; git clone -b "
        + branch_name
        + " https://"
        + github_token
        + "@github.com/whisthq/whist.git | tee ~/github_log.log"
    )
    description = (
        f"Cloning branch {branch_name} of the whisthq/whist repository on the AWS instance"
    )

    branch_not_found_error = f"fatal: Remote branch {branch_name} not found in upstream origin"
    branch_not_found_usr_msg = f"Branch {branch_name} not found in the whisthq/whist repository. Maybe it has been merged while the E2E was running?"
    general_clone_error = "fatal: "
    general_clone_usr_msg = f"Git clone failed!"

    errors_to_handle = [
        (branch_not_found_error, branch_not_found_usr_msg, None),
        (general_clone_error, general_clone_usr_msg, None),
    ]
    remote_executor.run_command(
        command,
        description=description,
        max_retries=SETUP_MAX_RETRIES,
        errors_to_handle=errors_to_handle,
    )

    print("Finished downloading whisthq/whist on EC2 instance")


def run_host_setup(remote_executor):
    """
    Run Whist's host setup on a remote machine accessible via a SSH connection within a pexpect process.

    The function assumes that the pexpect_process process has already successfully established a SSH
    connection to the host, and that the Whist repository has already been cloned.

    Args:
        remote_executor (RemoteExecutor):  The executor object to be used to run commands on the remote instance

    Returns:
        None
    """
    # 1- Ensure that the apt/dpkg locks are not taken by other processes
    wait_for_apt_locks(remote_executor)

    success_msg = "Install complete. If you set this machine up for local development, please 'sudo reboot' before continuing."
    lock_error_msg = "E: Could not get lock"
    lock_error_msg_usr = "Host setup failed to grab the necessary apt/dpkg locks."
    dpkg_config_error = "E: dpkg was interrupted, you must manually run 'sudo dpkg --configure -a' to correct the problem."
    dpkg_config_error_usr = (
        "Host setup failed due to dpkg interruption error. Reconfiguring dpkg...."
    )
    command = f"cd ~/whist/host-setup && timeout {HOST_SETUP_TIMEOUT_SECONDS} ./setup_host.sh --localdevelopment | tee ~/host_setup.log"
    description = f"Running the host setup on the instance"

    errors_to_handle = [
        (
            lock_error_msg,
            lock_error_msg_usr,
            "while sudo fuser /var/{lib/{dpkg,apt/lists},cache/apt/archives}/lock >/dev/null 2>&1; do echo 'Waiting for apt locks...'; sleep 1; done",
        ),
        (dpkg_config_error, dpkg_config_error_usr, "sudo dpkg --force-confdef --configure -a "),
    ]
    remote_executor.run_command(
        command,
        description=description,
        success_message=success_msg,
        max_retries=SETUP_MAX_RETRIES,
        errors_to_handle=errors_to_handle,
    )
    print("Finished running the host setup script on the EC2 instance")


def start_host_service(remote_executor):
    """
    Run Whist's host service on a remote machine accessible via a SSH connection within a pexpect process.

    The function assumes that the pexpect_process process has already successfully established a SSH connection
    to the host, and that the Whist repository has already been cloned.

    Args:
        remote_executor (RemoteExecutor):  The executor object to be used to run commands on the remote instance

    Returns:
        None
    """
    print("Starting the host service on the EC2 instance...")
    command = "sudo rm -rf /whist && cd ~/whist/backend/services && make run_host_service | tee ~/host_service.log"
    success_msg = "Entering event loop..."
    remote_executor.pexpect_process.sendline(command)

    result = remote_executor.pexpect_process.expect(
        [success_msg, remote_executor.pexpect_prompt, pexpect.exceptions.TIMEOUT, pexpect.EOF]
    )

    # If the desired output does not get printed, handle potential host service startup issues.
    if result != 0:
        exit_with_error("Host service failed to start! Check the logs for troubleshooting!")

    print("Host service is ready!")


def prune_containers_if_needed(remote_executor):
    """
    Check whether the remote instance is running out of space (more specifically,
    check if >= 75 % of disk space is used). If the disk is getting full, free some space
    by pruning old Docker containers by running the command `docker system prune -af`

    Args:
        remote_executor (RemoteExecutor):  The executor object to be used to run commands on the remote instance

    Returns:
        None
    """
    # Check if we are running out of space
    command = "df -h | grep --color=never /dev/root"
    remote_executor.run_command(command)
    for line in reversed(remote_executor.pexpect_output):
        if "/dev/root" in line:
            remote_executor.pexpect_output = line.split()
            break
    space_used_pctg = int(remote_executor.pexpect_output[-2][:-1])

    # Clean up space on the instance by pruning all Docker containers if the disk is 75% (or more) full
    if space_used_pctg >= 75:
        command, description = (
            "docker system prune -af",
            f"Disk is {space_used_pctg}% full, pruning the docker containers",
        )
        remote_executor.run_command(command, description)
    else:
        print(f"Disk is {space_used_pctg}% full, no need to prune containers.")
