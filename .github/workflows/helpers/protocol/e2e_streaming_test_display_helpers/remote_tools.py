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


def create_github_gist_post(github_gist_token, title, body):
    """
    Create a secret Github Gist for the E2E results. Add one file for each tuple
    in the body parameter. Print the html url of the secret Gist.
    Args:
        github_gist_token (str): The Github Gist token to use for authentication
        title (str): The title to give to the Gist
        body (str): A list of tuples, where each tuple contains the name of a file
                    to add to the Gist and the desired contents of the file
    Returns:
        None

    """
    # Display the results as a Github Gist
    client = Github(github_gist_token)
    gh_auth_user = client.get_user()
    files_dict = {}
    for t in body:
        files_dict[t[0]] = InputFileContent(t[1])
    gist = gh_auth_user.create_gist(
        public=False,
        files=files_dict,
        description=title,
    )
    print(f"Posted performance results to secret gist: {gist.html_url}")
    return gist.html_url


def create_slack_post(slack_webhook, title, gist_url):
    """
    Create a Slack post with the link to the newly-created Gist with the E2E results.
    The post is made in the channel pointed to by the Slack webhook
    Args:
        slack_webhook (str): The Slack token/url to use for posting
        title (str): The title to give to the Slack post
        gist_url (str): The contents of the Gist (the table(s) in markdown format)
    Returns:
        None
    """
    if slack_webhook:
        slack_post(
            slack_webhook,
            body=f"New E2E dev benchmark results available! Check them out here: {gist_url}\n",
            slack_username="Whist Bot",
            title=title,
        )


def search_open_PR(branch_name):
    """
    Check if there is an open PR associated with the specified branch. If so, return the PR number

    Args:
        branch_name (str): The name of the branch for which we are looking for an open PR
    Returns:
        On success:
            pr_number (int): The PR number of the open PR
        On failure:
            -1
    """
    # Search for an open PR connected to the current branch. If found, post results as a comment on that PR's page.
    result = ""
    if len(branch_name) > 0:
        gh_cmd = f"gh pr list -H {branch_name}"
        result = subprocess.check_output(gh_cmd, shell=True).decode("utf-8").strip().split()
    pr_number = -1
    if len(result) >= 3 and branch_name in result and result[0].isnumeric():
        pr_number = int(result[0])
    return pr_number
