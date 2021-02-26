#!/usr/bin/env python
import os
import json
from functools import wraps
import requests


def with_slack(func):
    """A decorator to pass a configured Slack client to functions

    Passes a default Slack client to decorated functions as the
    'client=' kwarg.

    Args:
        func: A function that accepts a Slack client as its first argument.
    Returns:
        The input function, partially applied with the Slack client object.
    """

    @wraps(func)
    def with_slack_wrapper(*args, **kwargs):

        slack_username = os.environ.get("SLACK_USERNAME", "Fractal Bot")
        slack_url = os.environ.get("SLACK_URL")

        if not slack_url:
            raise Exception("No SLACK_URL environment variable provided.")

        kwargs["url"] = kwargs.get("url", slack_url)
        kwargs["username"] = kwargs.get("username", slack_username)

        return func(*args, **kwargs)

    return with_slack_wrapper


def validate_slack_config(func):
    @wraps(func)
    def validate_slack_config_wrapper(*args, **kwargs):
        username = kwargs.get("username")
        if not isinstance(username, str):
            raise Exception(
                "Slack client must be configured with "
                + "a 'username=' keyword argument as a string. "
                + f"Received: {username}")

        return func(*args, **kwargs)

    return validate_slack_config_wrapper


@with_slack
@validate_slack_config
def create_post(channel, body, url=None, username=None, text=None):
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
            "text": text}

    r = requests.post(url,
                      data=json.dumps(data),
                      headers={"Content-Type": "application/json"})

    r.raise_for_status()

    return r
