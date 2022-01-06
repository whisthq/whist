#!/usr/bin/env python3

import pexpect
import os
import time
import subprocess


def attempt_ssh_connection(ssh_command, timeout, log_file_handle, pexpect_prompt, max_retries):
    for retries in range(max_retries):
        child = pexpect.spawn(ssh_command, timeout=timeout, logfile=log_file_handle.buffer)
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


def wait_until_cmd_done(ssh_proc, pexpect_prompt):
    # On a SSH connection, the prompt is printed two times (because of some obscure reason related to encoding and/or color printing on terminal)
    ssh_proc.expect(pexpect_prompt)
    ssh_proc.expect(pexpect_prompt)


def reboot_instance(
    ssh_process, connection_ssh_cmd, timeout, log_file_handle, pexpect_prompt, retries
):
    ssh_process.sendline(" ")
    wait_until_cmd_done(ssh_process, pexpect_prompt)
    ssh_process.sendline("sudo reboot")
    ssh_process.kill(0)
    time.sleep(5)
    ssh_process = attempt_ssh_connection(
        connection_ssh_cmd, timeout, log_file_handle, pexpect_prompt, retries
    )
    print("Reboot complete")
    return ssh_process


def apply_dpkg_locking_fixup(pexpect_process, pexpect_prompt):
    ## Prevent dpkg locking issues such as the following one:
    # E: Could not get lock /var/lib/dpkg/lock-frontend. It is held by process 2392 (apt-get)
    # E: Unable to acquire the dpkg frontend lock (/var/lib/dpkg/lock-frontend), is another process using it?
    # See also here: https://github.com/ray-project/ray/blob/master/doc/examples/lm/lm-cluster.yaml
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
        wait_until_cmd_done(pexpect_process, pexpect_prompt)


def configure_aws_credentials(
    pexpect_process, pexpect_prompt, aws_credentials_filepath="~/.aws/credentials"
):

    aws_credentials_filepath_expanded = os.path.expanduser(aws_credentials_filepath)

    if not os.path.isfile(aws_credentials_filepath_expanded):
        print(
            "Could not find local AWS credential file at path {}!".format(aws_credentials_filepath)
        )
        return

    aws_credentials_file = open(aws_credentials_filepath_expanded, "r")
    aws_access_key_id = ""
    aws_secret_access_key = ""
    for line in aws_credentials_file.readlines():
        if "aws_access_key_id" in line:
            aws_access_key_id = line.strip().split()[2]
        elif "aws_secret_access_key" in line:
            aws_secret_access_key = line.strip().split()[2]
            break
    if aws_access_key_id == "" or aws_secret_access_key == "":
        print(
            "Could not parse AWS credentials from file at path {}!".format(aws_credentials_filepath)
        )
        return
    pexpect_process.sendline("sudo apt-get update")
    result = pexpect_process.expect(["Do you want to continue?", pexpect_prompt])
    if result == 0:
        pexpect_process.sendline("Y")
        wait_until_cmd_done(pexpect_process, pexpect_prompt)
    else:
        pexpect_process.expect(pexpect_prompt)
    pexpect_process.sendline("sudo apt-get install awscli")
    result = pexpect_process.expect(["Do you want to continue?", pexpect_prompt])
    if result == 0:
        pexpect_process.sendline("Y")
        wait_until_cmd_done(pexpect_process, pexpect_prompt)
    else:
        pexpect_process.expect(pexpect_prompt)
    pexpect_process.sendline("aws configure")
    pexpect_process.expect("AWS Access Key ID")
    pexpect_process.sendline(aws_access_key_id)
    pexpect_process.expect("AWS Secret Access Key")
    pexpect_process.sendline(aws_secret_access_key)
    pexpect_process.expect("Default region name")
    pexpect_process.sendline("")
    pexpect_process.expect("Default output format")
    pexpect_process.sendline("")
    wait_until_cmd_done(pexpect_process, pexpect_prompt)
    aws_credentials_file.close()


def clone_whist_repository_on_instance(github_token, pexpect_process, pexpect_prompt):
    # Obtain current branch
    subproc_handle = subprocess.Popen("git branch", shell=True, stdout=subprocess.PIPE)
    subprocess_stdout = subproc_handle.stdout.readlines()

    branch_name = ""
    for line in subprocess_stdout:
        converted_line = line.decode("utf-8").strip()
        if "*" in converted_line:
            branch_name = converted_line[2:]
            break

    print(
        "Cloning branch {} of the whisthg/whist repository on the AWS instance ...".format(
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
    wait_until_cmd_done(pexpect_process, pexpect_prompt)
    print("Finished downloading whisthq/whist on EC2 instance")
