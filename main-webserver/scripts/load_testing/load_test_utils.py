"""
This file contains common load test utility constants/functions.
"""

import sys
import os


LOAD_TEST_USER_PREFIX = "load_test_user_{user_num}"


def get_task_definition_arn(web_url: str) -> str:
    """
    Get the task definition to run. Depending on the `web_url`, we
    figure out whether we should run the `dev` or `staging`
    chrome task definitions.

    Args:
        web_url: URL of the webserver to load test

    Returns:
        string of the form fractal-<stage>-browsers-chrome
    """
    if "dev" in web_url or "localhost" in web_url:
        return "fractal-dev-browsers-chrome"
    elif "staging" in web_url:
        return "fractal-staging-browsers-chrome"
    else:
        # use staging by default
        return "fractal-staging-browsers-chrome"
