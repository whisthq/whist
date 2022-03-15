#!/usr/bin/env python3

import os
import sys
import subprocess

GITHUB_SHA_LEN = 40

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


def get_whist_branch_name(running_in_ci):
    """
    Retrieve the whisthq/whist branch name currently checked out by the folder containing this script.

    Args:
        running_in_ci: A boolean indicating whether this script is currently running in CI

    Returns:
        On success:
            branch_name: The name of the branch currently checked out (or the empty string)
        On failure:
            empty string
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
        running_in_ci: A boolean indicating whether this script is currently running in CI

    Returns:
        On success:
            github_sha: The SHA commit hash
        On failure:
            empty string
    """
    github_sha = ""

    if running_in_ci:
        # In CI, the Github SHA is saved in the GITHUB_SHA environment variable
        github_sha = os.getenv("GITHUB_SHA")
    else:
        # Locally, we can find the branch using the 'git rev-parse HEAD' command.
        subproc_handle = subprocess.Popen("git rev-parse HEAD", shell=True, stdout=subprocess.PIPE)
        subprocess_stdout = subproc_handle.stdout.readlines()
        converted_line = subprocess_stdout[0].decode("utf-8").strip()
        if len(converted_line) == GITHUB_SHA_LEN:
            github_sha = converted_line

    return github_sha
