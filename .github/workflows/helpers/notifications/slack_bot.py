#!/usr/bin/env python3
"""
This file contains a function `slack_post` to post messages to Slack. It is typically
used in our GitHub Actions workflows and only requires the requests Python package. This
package comes pre-installed on GHA, so we can use this out-of-box with little chance
of things going wrong.
"""

import os
import json
import sys

import requests
from . import formatters as fmt

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


def slack_post(
    slack_webhook,
    body,
    slack_username="Whist Bot",
    title=None,
    code=None,
    lang=None,
):
    """Creates a post on a Slack channel

    Args:
        slack_webhook: a string, Slack webhook to send messages to
        body: a string, the main content of the comment
        slack_username: an optional string, Name of account to post messages as
        title: an optional string, formatted at the top of the comment
        code: an optional string, placed in a block at the bottom of the comment
        lang: an optional string, used to format the comment's code block
    Returns
        requests.Response object
    """
    body = fmt.default_message_slack(body, title, code, lang)
    data = {
        "username": slack_username,
        "icon_emoji": ":whist:",
        "link_names": 1,  # this makes @channel, @whist_employee work
        "type": "mrkdwn",
        "text": body,
    }
    resp = requests.post(
        slack_webhook,
        data=json.dumps(data),
        headers={"Content-Type": "application/json"},
    )
    resp.raise_for_status()
    return resp
