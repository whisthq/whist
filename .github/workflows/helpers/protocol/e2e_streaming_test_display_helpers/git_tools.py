#!/usr/bin/env python3

import os
import sys
import subprocess
from github import Github, InputFileContent, GithubException

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


def get_gist_user_info(github_gist_token):
    """
    Get the username, name and primary email associated with a particular git token. We use the username to
    obtain the link to images that are uploaded to a Gist file. We use the name and email to commit the images
    to the Gist

    Args:
        github_gist_token (str):    The Github Gist token to use for authentication
    Returns:
        username (str): The Github username
        name (str): The name associated with the Github account
        email (str): The email associated with the Github account
    """
    client = Github(github_gist_token)
    user = client.get_user()
    username = user.login
    name = user.name
    try:
        email = list(filter(lambda d: d.primary == True, user.get_emails()))[0].email
    except GithubException as e:
        print("Caught exception: ", e)
        email = "e2e_bot@whist.com"
    return username, name, email


def create_or_update_gist(github_gist_token, gist=None, title=None, files_list=None, verbose=False):
    # either existing_gist or title should be defined
    if not gist and not title:
        print(
            "Error: gist and title are both `None`. To create a new gist, pass a title. To update a gist, pass the handle to an existing gist"
        )
        return None
    if not gist:
        client = Github(github_gist_token)
        gh_auth_user = client.get_user()
        gist = gh_auth_user.create_gist(
            public=False,
            files={
                "placeholder.txt": InputFileContent(
                    "If you can see this, the Gist was created successfully!"
                )
            },
            description=title,
        )
        if not gist:
            print(f"Error: could not create gist with title {title}")
            return None
        print(f"\nInitialized secret gist at url: {gist.html_url}")

    if files_list:
        files = " ".join(files_list)
        print(f"Updating gist {gist.id} with files {files}")
        upload_files_command = f"gist-paste -u {gist.id} {files}"
        if not verbose:
            subprocess.run(
                upload_files_command,
                shell=True,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.STDOUT,
            )
        else:
            subprocess.run(upload_files_command, shell=True, capture_output=True)

    return gist


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
