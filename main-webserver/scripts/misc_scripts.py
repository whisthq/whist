import argparse
import sys
import os

# this adds the webserver repo root to the python path no matter where
# this file is called from. We can now import from `scripts`.
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), ".."))

from scripts.utils import make_post_request

# all currently supported scripts
SUPPORTED_SCRIPTS = ["login"]

# scripts that need admin token to run
SCRIPTS_NEEDING_ADMIN = []


def login(web_url: str, username: str, password: str):
    """
    Login and get the access tokens for a particular user.

    Args:
        web_url: URL to run script on
        username: user to login as
        password: password associated with user

    Returns:
        dict containing "access_token", "refresh_token"
    """
    endpoint = "/admin/login"
    payload = {
        "username": username,
        "password": password,
    }
    resp = make_post_request(web_url, endpoint, payload)
    assert resp.status_code == 200, f"got code: {resp.status_code}, content: {resp.content}"
    resp_json = resp.json()
    return resp_json
