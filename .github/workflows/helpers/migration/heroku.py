##!/usr/bin/env python3
import requests
import os
import sys
from pprint import pprint

HEROKU_BASE_URL = "https://api.heroku.com"
HEROKU_API_TOKEN = os.environ.get("HEROKU_API_TOKEN")

def config_vars(app_name):
    """Retrieves a map of configuration variables from Heroku app

    Given an app name, makes a call to the Heroku API to ask for all
    configuration variables stored under that app. This function relies on a
    valid API in the environement variable HEROKU_API_TOKEN.

    Args:
        app_name: A string that represents the name of the app, and configured
                  in Heroku.
    Returns:
        A dictionary of the configuration variable names and values in Heroku.
    """
    url = f"{HEROKU_BASE_URL}/apps/{app_name}/config-vars"


    r = requests.get(url,
                     headers={
                         "Accept": "application/vnd.heroku+json; version=3",
                         "Authorization": f"Bearer {HEROKU_API_TOKEN}"
                     })

    r.raise_for_status()
    return r.json()

if __name__ == "__main__":
    _, app_name, *keys = sys.argv

    config = config_vars(app_name)

    # if not keys:
    #     pprint(config)

    print(*(config.get(f) for f in keys if f))
