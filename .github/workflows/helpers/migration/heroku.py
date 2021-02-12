#!/usr/bin/env python3
import requests
import inspect
import config
from pprint import pprint
from errors import catch_auth_error

# heroku credentials should already be in .netrc before this script runs
@catch_auth_error
def config_vars(app_name):
    url = f"{config.HEROKU_BASE_URL}/apps/{app_name}/config-vars"

    r = requests.get(
        url,
        headers={"Accept": "application/vnd.heroku+json; version=3"})

    r.raise_for_status()
    return r.json()
