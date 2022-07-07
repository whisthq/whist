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
        email = list(filter(lambda d: d["primary"] == True, user.get_emails()))[0]["email"]
    except GithubException as e:
        print("Caught exception: ", e)
        email = "e2e_bot@whist.com"
    return username, name, email


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
        files={
            "placeholder.txt": InputFileContent(
                "If you can see this, the Gist was created successfully!"
            )
        },
        description=title,
    )
    print(f"\nInitialized secret gist for the performance results: {gist.html_url}")
    return gist


def update_github_gist_post(github_gist_token, gist_id, files_list, verbose):
    """
    Update a secret Github Gist with the E2E results. Add all files at the path provided
    in the files_list list.

    Args:
        github_gist_token (str):    The Github Gist token to use for authentication
        gist_id (str): The ID of the Github Gist object that was previously created
        files_list (list):  A list of files to be uploaded to the Gist post
        verbose (bool): Whether to print verbose logs
    Returns:
        None
    """
    # Clone the gist
    clone_command = f"git clone https://{github_gist_token}@gist.github.com/{gist_id}"
    if not verbose:
        os.system(clone_command)
    else:
        subprocess.run(clone_command, shell=True, capture_output=True)
    # Copy the plots and reports into the gist folder
    for old_filepath in files_list:
        os.replace(old_filepath, os.path.join(".", f"{gist_id}", os.path.basename(old_filepath)))
    # Upload all the files
    upload_files_command = f"cd {gist_id} && rm placeholder.txt && git add * && git commit -am 'upload files' && git push"
    if not verbose:
        os.system(upload_files_command)
    else:
        subprocess.run(upload_files_command, shell=True, capture_output=True)


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
