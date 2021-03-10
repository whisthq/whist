import os
import argparse
import json
from typing import List

import requests


SUPPORTED_SCRIPTS = [
    "login",
    "poll_celery_task",
    "start_maintenance",
    "end_maintenance",
    "update_region",
    "update_taskdefs",
]

SCRIPTS_NEEDING_ADMIN = [
    "start_maintenance",
    "end_maintenance",
    "update_region",
    "update_taskdefs",
]

def make_post_request(
        web_url: str,
        endpoint: str,
        payload: dict,
        admin_token: str = None,
    ) -> requests.Response:
    # this neatly handlers trailing slashes and such
    url = os.path.join(web_url, endpoint)
    headers = {
        "Authorization": f"Bearer {admin_token}",
    }
    payload_str = json.dumps(payload)
    return requests.post(url=url, headers=headers, data=payload_str)

def handle_login(web_url: str):
    login_parser = argparse.ArgumentParser(description="Run a login parser.")
    login_parser.add_argument("--username", type=str, help="Username in payload.")
    login_parser.add_argument("--password", type=str, help="Password in payload.")
    args, _ = login_parser.parse_known_args()
    endpoint = "/admin/login"
    payload = {
        "username": args.username,
        "password": args.password,
    }
    resp = make_post_request(web_url, endpoint, payload)
    assert resp.status_code == 200
    # print it so that bash scripts can extract stdout and use it
    print(resp.content)


def run_script(web_url: str, method: str, admin_token: str=None):
    assert method in SUPPORTED_SCRIPTS
    if admin_token is None:
        # if no admin token, make sure the method does not need an admin token
        assert method not in SCRIPTS_NEEDING_ADMIN 
    
    # dispatch based on method
    if method == "login":
        return handle_login(web_url)
    elif method == "poll_celery_task":
        return handle_poll_celery_task(web_url, admin_token)
    elif method == "start_maintenance":
        return handle_start_maintenance(web_url, admin_token)
    elif method == "end_maintenance":
        return handle_end_maintenance(web_url, admin_token)
    elif method == "update_region":
        return handle_update_region(web_url, admin_token)
    elif method == "update_taskdefs":
        return handle_update_taskdefs(web_url, admin_token)
    else:
        raise ValueError(f"Unknown method {method}")


if __name__ == '__main__':
    # generic arguments, specific scripts might request more
    parser = argparse.ArgumentParser(description="Run a supported webserver script.")

    parser.add_argument("--web_url", type=str, help="URL of webserver to hit.")
    parser.add_argument(
        "--script", type=str, help=f"Method to invoke. Supported choices: {SUPPORTED_SCRIPTS}"
    )
    parser.add_argument(
        "--admin_token", type=str, default=None, help="Admin token for special permissions."
    )

    args, _ = parser.parse_known_args()
    run_script(args.web_url, args.script, args.admin_token)
