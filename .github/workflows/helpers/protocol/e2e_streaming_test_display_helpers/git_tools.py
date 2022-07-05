#!/usr/bin/env python3

import os
import sys
import subprocess
from github import Github, InputFileContent

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


def initialize_github_gist_post(github_gist_token, title):
    """
    Create a secret Github Gist for the E2E results. Initialize the Gist's description

    Args:
        github_gist_token (str):    The Github Gist token to use for authentication
        title (str):    The title to give to the Gist
    Returns:
        gist (github.Gist.Gist): The Github Gist object just created
    """
    client = Github(github_gist_token)
    gh_auth_user = client.get_user()
    gist = gh_auth_user.create_gist(
        public=False,
        description=title,
    )
    print(f"\nInitialized secret gist for the performance results: {gist.html_url}")
    return gist


def update_github_gist_post(gist, files_list):
    """
    Update a secret Github Gist with the E2E results. Add one file for each tuple
    in the files_list parameter. Print the html url of the secret Gist.

    Args:
        github_gist_token (str):    The Github Gist token to use for authentication
        gist (github.Gist.Gist): The Github Gist object that was previously created
        files_list (list):  A list of tuples, where each tuple contains the name of a file
                            to add to the Gist and the desired contents of the file
    Returns:
        None
    """
    if not gist:
        print("Error: Could not update Gist!")

    files_dict = {}
    for filename, file_contents in files_list:
        files_dict[filename] = InputFileContent(file_contents)
    gist.edit(description=gist.description, files=files_dict)
    print(f"\nPosted performance results to secret gist: {gist.html_url}")
    return gist.html_url


def associate_branch_to_open_pr(branch_name, verbose=False):
    """
    Check if there is an open PR associated with the specified branch. If so, return the PR number

    Args:
        branch_name (str): The name of the branch for which we are looking for an open PR
        verbose (bool): Whether to print verbose logs to stdout

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
            return_value = int(pr_number)
            if verbose:
                print(
                    f"Found open PR #{pr_number}: '{pr_name}' associated with branch '{branch_name}'!"
                )

    if return_value == -1 and verbose:
        print(f"Found no open PR associated with branch '{branch_name}'!")

    return return_value
