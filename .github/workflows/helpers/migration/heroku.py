#!/usr/bin/env python3
import requests
import inspect
import config
from pprint import pprint
from errors import catch_auth_error


@catch_auth_error
def config_vars(app_name):
    """Retrieves a map of configuration variables from Heroku app

    Given an app name, makes a call to the Heroku API to ask for all
    configuration variables stored under that app. This function relies
    on ~/.netrc for authentication, which can either be configured manually
    in CI/CD or be configured interactively with `heroku login` in the CLI.
    """
    url = f"{config.HEROKU_BASE_URL}/apps/{app_name}/config-vars"

    r = requests.get(url, headers={"Accept": "application/vnd.heroku+json; version=3"})

    r.raise_for_status()
    return r.json()
