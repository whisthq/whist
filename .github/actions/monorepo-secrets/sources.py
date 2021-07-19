#!/usr/bin/env python

import sys
import requests

HEROKU_BASE_URL = "https://api.heroku.com"


def heroku(secrets):
    """Retrieves a map of configuration variables from Heroku app

    Given an app name, makes a call to the Heroku API to ask for all
    configuration variables stored under that app. This function relies on a
    valid API in the environement variable HEROKU_API_TOKEN.

    Args:
        secrets: A dictionary of secret keys and values. Must include keys:
                 - "HEROKU_APP_NAME"
                 - "HEROKU_API_TOKEN"

    Returns:
        A dictionary of the configuration variable names and values in Heroku.
    """
    if not secrets.get("HEROKU_APP_NAME"):
        raise Exception("Secrets map must contain key: HEROKU_APP_NAME")
    if not secrets.get("HEROKU_API_TOKEN"):
        raise Exception("Secrets map must contain key: HEROKU_API_TOKEN")

    app_name = secrets.get("HEROKU_APP_NAME")
    api_token = secrets.get("HEROKU_API_TOKEN")

    url = f"{HEROKU_BASE_URL}/apps/{app_name}/config-vars"

    resp = requests.get(
        url,
        headers={
            "Accept": "application/vnd.heroku+json; version=3",
            "Authorization": f"Bearer {api_token}",
        },
    )

    resp.raise_for_status()
    return resp.json()
