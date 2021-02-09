#!/usr/bin/env python3
#
import requests

# heroku credentials should already be in .netrc before this script runs
def config_vars(app_name):
    url = f"https://api.heroku.com/apps/{app_name}/config-vars"

    r = requests.get(
        url,
        headers={"Accept": "application/vnd.heroku+json; version=3"})

    return r.json()
