#!/usr/bin/env python
"""
This file contains a function `slack_post` to post messages to Slack. It is typically
used in our Github Actions workflows and only requires the requests Python package. This
package comes pre-installed on GHA, so we can use this out-of-box with little chance
of things going wrong.
"""

import os
import json
from functools import wraps
import sys

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))

import requests

import formatters as fmt


def slack_post(
    slack_webhook,
    channel,
    body,
    slack_username="Fractal Bot",
    title=None,
    code=None,
    lang=None,
):
    """Creates a post on a Slack channel

    Args:
        slack_webhook: a string, Slack webhook to send messages to
        channel: a string, Slack channel to post to
        body: a string, the main content of the comment
        slack_username: an optional string, Name of account to post messages as
        title: an optional string, formatted at the top of the comment
        code: an optional string, placed in a block at the bottom of the comment
        lang: an optional string, used to format the comment's code block
    Returns
        None"""
    fmt_body = fmt.default_message_slack(body, title, code, lang)
    _create_post(
        slack_webhook=slack_webhook,
        slack_username=slack_username,
        channel=channel,
        body=fmt_body,
    )


def _create_post(slack_webhook, slack_username, channel, body):
    """
    Args:
        slack_webhook: a string, Slack webhook to send messages to
        slack_username: a string, Name of account to post messages as
        channel: a string, Slack channel to post to
        body: a string, markdown Slack message to post
    Returns
        Errors out if got bad response from Slack.
    """
    data = {
        "channel": channel,
        "username": slack_username,
        "icon_emoji": ":fractal:",
        "link_names": 1,  # this makes @channel, @fractal_employee work
        "type": "mrkdwn",
        "text": body,
    }
    r = requests.post(
        slack_webhook,
        data=json.dumps(data),
        headers={"Content-Type": "application/json"},
    )
    r.raise_for_status()
    return r
