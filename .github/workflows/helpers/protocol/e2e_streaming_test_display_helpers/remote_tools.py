#!/usr/bin/env python3

import os
import sys
import subprocess
from pytablewriter import MarkdownTableWriter
from contextlib import redirect_stdout
from datetime import datetime, timedelta

from github import Github, InputFileContent
from notifications.slack_bot import slack_post
from notifications.github_bot import github_comment_update

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


def create_github_gist_post(github_gist_token, title, files_list):
    """
    Create a secret Github Gist for the E2E results. Add one file for each tuple
    in the files_list parameter. Print the html url of the secret Gist.
    Args:
        github_gist_token (str): The Github Gist token to use for authentication
        title (str): The title to give to the Gist
        files_list (str): A list of tuples, where each tuple contains the name of a file
                    to add to the Gist and the desired contents of the file
    Returns:
        None

    """
    # Display the results as a Github Gist
    client = Github(github_gist_token)
    gh_auth_user = client.get_user()
    files_dict = {}
    for filename, file_contents in files_list:
        files_dict[filename] = InputFileContent(file_contents)
    gist = gh_auth_user.create_gist(
        public=False,
        files=files_dict,
        description=title,
    )
    print(f"Posted performance results to secret gist: {gist.html_url}")
    return gist.html_url


def associate_branch_to_open_pr(branch_name):
    """
    Check if there is an open PR associated with the specified branch. If so, return the PR number

    Args:
        branch_name (str): The name of the branch for which we are looking for an open PR
    Returns:
        On success (an open PR is found):
            pr_number (int): The PR number of the open PR
        On failure (no open PR for the branch is found):
            -1
    """
    # Use Github CLI to search for an open PR connected to the current branch
    result = ""
    if len(branch_name) > 0:
        gh_cmd = f"gh pr list -H {branch_name}"
        result = subprocess.check_output(gh_cmd, shell=True).decode("utf-8").strip().split("\t")

    return_value = -1

    # On success, result contains: ["<PR number>", "<PR name>", "<PR branch name>", "<DRAFT> if this is a draft PR"]
    # On failure, success is a list containing an empty string: ['']
    if len(result) >= 3:
        pr_number, pr_name, pr_branch_name = result[:3]
        if pr_number.isnumeric() and branch_name == pr_branch_name:
            print(
                f"Found open PR #{pr_number}: '{pr_name}' associated with branch '{branch_name}'!"
            )
            return_value = int(pr_number)

    if return_value == -1:
        print(f"Found no open PR associated with branch '{branch_name}'!")

    return return_value
