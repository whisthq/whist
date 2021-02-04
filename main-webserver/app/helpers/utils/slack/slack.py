import json
import logging

import requests

from app.helpers.utils.general.logs import fractal_log

def _slack_send(channel: str, message: str):
    # all channels start with a #
    assert channel[0] == "#"
    url = "https://hooks.slack.com/services/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"
    payload = {
        "channel": channel,
        "username": "Fractal Bot",
        "text": message,
        "icon_emoji": ":fractal:",
    }

    resp = requests.post(url, json.dumps(payload))
    assert resp.status_code == 200


def slack_send_safe(channel: str, message: str):
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

