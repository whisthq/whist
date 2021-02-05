import json
import logging

import requests
from flask import current_app

from app.helpers.utils.general.logs import fractal_log


def _slack_send(channel: str, message: str):
    """
    Send a message to a channel on Fractal slack as Fractal Bot. This can fail out with asserts.

    Args:
        channel: channel to post in. Must start with #. Example: #alerts-test
        message: message to send.
    """

    # all channels start with a #
    assert channel[0] == "#"
    url = current_app.config["SLACK_WEBHOOK"]
    payload = {
        "channel": channel,
        "username": "Fractal Bot",
        "text": message,
        "icon_emoji": ":fractal:",
    }

    resp = requests.post(url, json.dumps(payload))
    assert resp.status_code == 200


def slack_send_safe(channel: str, message: str):
    """
    Send a message to a channel on Fractal slack as Fractal Bot. This is a wrapper around _slack_send
    and safely handles exceptions during production.

    Args:
        channel: channel to post in. Must start with #. Example: #alerts-test
        message: message to send.
    """
    try:
        _slack_send(channel, message)
    except Exception as e:
        try:
            exc_str = str(e)
            fractal_log(
                "slack_send_safe",
                None,
                f"Failed to send slack message {message} to channel {channel}. Exception string: {exc_str}.",
                level=logging.ERROR,
            )

        except:
            exc_str = str(e)
            fractal_log(
                "slack_send_safe",
                None,
                f"Failed to send slack message {message} to channel {channel}. Raised exception could not be serialized.",
                level=logging.ERROR,
            )
