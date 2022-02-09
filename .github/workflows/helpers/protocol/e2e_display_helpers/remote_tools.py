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
    # Display the results as a Github Gist
    client = Github(github_gist_token)
    gh_auth_user = client.get_user()
    gist = gh_auth_user.create_gist(
        public=False,
        files={
            "performance_results.md": InputFileContent(body),
        },
        description=title,
    )
    print("Posted performance results to secret gist: {}".format(gist.html_url))
    return gist.html_url


def create_slack_post(slack_webhook, title, gist_url):
    if slack_webhook:
        slack_post(
            slack_webhook,
            body="Daily E2E dev benchmark results: {}\n".format(gist_url),
            slack_username="Whist Bot",
            title=title,
        )


def search_open_PR():
    branch_name = ""

    # In CI, the PR branch name is saved in GITHUB_REF_NAME, or in the GITHUB_HEAD_REF environment variable (in case this script is being run as part of a PR)
    b = os.getenv("GITHUB_REF_NAME").split("/")
    if len(b) != 2 or not b[0].isnumeric() or b[1] != "merge":
        branch_name = os.getenv("GITHUB_REF_NAME")
    else:
        branch_name = os.getenv("GITHUB_HEAD_REF")

    # Search for an open PR connected to the current branch. If found, post results as a comment on that PR's page.
    result = ""
    if len(branch_name) > 0:
        gh_cmd = "gh pr list -H {}".format(branch_name)
        result = subprocess.check_output(gh_cmd, shell=True).decode("utf-8").strip().split()
    pr_number = -1
    if len(result) >= 3 and branch_name in result and result[0].isnumeric():
        pr_number = int(result[0])
    return pr_number
