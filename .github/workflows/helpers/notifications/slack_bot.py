#!/usr/bin/env python
import os
import json
from functools import wraps
import requests


def create_post(channel, body, slack_webhook, username):
    """
    Accepts a text= keyword argument, because the Slack SDK warns without
    passing it.
    """
    data = {"channel": channel,
            "username": username,
            "icon_emoji": ":fractal:",
            "blocks": [{"type": "section",
                       "text": {"type": "mrkdwn",
                                "text": body}}],
            "link_names": 1, # @channel, @fractal_employee work
            }

    r = requests.post(slack_webhook,
                      data=json.dumps(data),
                      headers={"Content-Type": "application/json"})

    r.raise_for_status()

    return r
