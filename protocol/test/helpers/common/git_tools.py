#!/usr/bin/env python3

import os, sys, subprocess

from helpers.common.pexpect_tools import wait_until_cmd_done
from helpers.common.timestamps_and_exit_tools import printred

# Add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))

# Constants
GITHUB_SHA_LEN = 40


def get_whist_branch_name(running_in_ci):
    """
    Retrieve the whisthq/whist branch name currently checked out by the folder containing this script.

    Args:
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI

    Returns:
        On success:
            branch_name (str): The name of the branch currently checked out (or the empty string)
        On failure:
            Empty string (str)
    """
    branch_name = ""

    if running_in_ci:
        # In CI, the PR branch name is saved in GITHUB_REF_NAME, or in the GITHUB_HEAD_REF environment
        # variable (in case this script is being run as part of a PR)
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


def get_whist_github_sha(running_in_ci):
    """
    Retrieve the commit hash of the latest commit in the local repository.

    Args:
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI

    Returns:
        On success:
            github_sha (str): The SHA commit hash
        On failure:
            empty string (str)
    """
    github_sha = ""

    if running_in_ci:
        # In CI, we save the commit hash in the variable GITHUB_COMMIT_HASH
        github_sha = os.getenv("GITHUB_COMMIT_HASH")
    else:
        # Locally, we can find the branch using the 'git rev-parse HEAD' command.
        subproc_handle = subprocess.Popen("git rev-parse HEAD", shell=True, stdout=subprocess.PIPE)
        subprocess_stdout = subproc_handle.stdout.readlines()
        converted_line = subprocess_stdout[0].decode("utf-8").strip()
        if len(converted_line) == GITHUB_SHA_LEN:
            github_sha = converted_line

    return github_sha


def get_remote_whist_github_sha(pexpect_process, pexpect_prompt, running_in_ci):
    """
    Retrieve the commit hash of the latest commit in the repository cloned on a remote isntance.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn):  The Pexpect process created with pexpect.spawn(...) and to
                                                    be used to interact with the remote machine
        pexpect_prompt: The bash prompt printed by the shell on the remote machine when it
                        is ready to execute a command
        running_in_ci: A boolean indicating whether this script is currently running in CI

    Returns:
        On success:
            github_sha (str): The SHA commit hash
        On failure:
            empty string (str)
    """

    pexpect_process.sendline("cd ~/whist && git rev-parse HEAD")
    stdout = wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci, return_output=True)
    if len(stdout) != 2 or len(stdout[-1]) != GITHUB_SHA_LEN:
        print(stdout)
        printred("Could not get the Github SHA of the commit checked out on the remote instance")
        return ""
    return stdout[-1]
