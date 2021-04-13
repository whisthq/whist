#!/usr/bin/env python
import os
import json
from functools import wraps
import requests


def create_post(slack_webhook, slack_username, channel, body):
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
