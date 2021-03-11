import os
import argparse
import time
import json
from typing import List
from urllib.parse import urljoin

import requests


# all currently supported scripts
SUPPORTED_SCRIPTS = [
    "login",
    "poll_celery_task",
    "start_maintenance",
    "end_maintenance",
    "update_region",
]

# scripts that need admin token to run
SCRIPTS_NEEDING_ADMIN = [
    "start_maintenance",
    "end_maintenance",
    "update_region",
]


def make_get_request(web_url: str, endpoint: str, admin_token: str = None):
    """
    Makes a GET request. Properly formats admin_token (if given).

    Args:
        web_url: URL to run script on
        endpoint: endpoint at URL to hit
        admin_token: Optional token for admin access

    Returns:
        Response from requests library.
    """
    url = urljoin(web_url, endpoint)
    headers = None
    if admin_token is not None:
        headers = {
            "Authorization": f"Bearer {admin_token}",
        }
    return requests.get(url=url, headers=headers)


def make_post_request(
    web_url: str,
    endpoint: str,
    payload: dict,
    admin_token: str = None,
) -> requests.Response:
    """
    Makes a POST request. Properly formats admin_token (if given) and payload.

    Args:
        web_url: URL to run script on
        endpoint: endpoint at URL to hit
        payload: data to POST to server. Can be None.
        admin_token: Optional token for admin access

    Returns:
        Response from requests library.
    """
    url = urljoin(web_url, endpoint)
    headers = None
    if admin_token is not None:
        headers = {
            "Authorization": f"Bearer {admin_token}",
        }
    payload_str = None
    if payload is not None:
        payload_str = json.dumps(payload)
    return requests.post(url=url, headers=headers, data=payload_str)


def handle_login(web_url: str) -> dict:
    """
    Handle login script. Steps:
    1. Parse `username` and `password` args.
    2. Make a login request
    3. Print response and return it.
    Errors out if anything goes wrong.

    Args:
        web_url: URL to run script on
        admin_token: Needed to authorize use of the update_region endpoint.

    Returns:
        dict containing "access_token", "refresh_token"
    """
    login_parser = argparse.ArgumentParser(description="Run a login parser.")
    login_parser.add_argument("--username", type=str, required=True, help="Username in payload.")
    login_parser.add_argument("--password", type=str, required=True, help="Password in payload.")
    args, _ = login_parser.parse_known_args()

    endpoint = "/admin/login"
    payload = {
        "username": args.username,
        "password": args.password,
    }
    resp = make_post_request(web_url, endpoint, payload)
    assert resp.status_code == 200, f"got code: {resp.status_code}, content: {resp.content}"
    # print raw JSON (neatly) so that bash scripts can extract stdout and use it
    print(resp.content.decode("utf-8").strip())
    resp_json = resp.json()
    return resp_json


def poll_celery_task(web_url: str, task_id: str, admin_token: str = None) -> str:
    """
    Poll celery task `task_id` for up to 10 minutes.
    Args:
        web_url: URL to run script on
        task_id: Task to poll
        admin_token: Optional; can provide traceback info.

    Returns:
        The task output on success, otherwise errors out.
    """
    # 300 * 2 sec = 600 sec = 10 min
    num_tries = 300
    sleep_time = 2
    endpoint = f"/status/{task_id}"
    resp_json = None
    for _ in range(num_tries):
        resp = make_get_request(web_url, endpoint, admin_token=admin_token)
        assert resp.status_code == 200, f"got code: {resp.status_code}, content: {resp.content}"
        resp_json = resp.json()
        if resp_json["state"] in ("PENDING", "STARTED"):
            # sleep and try again
            time.sleep(sleep_time)
        else:
            # non-pending/started states should exit
            break
    assert resp_json is not None
    assert resp_json["state"] == "SUCCESS", f"got response: {resp_json}"
    return resp_json["output"]


def handle_poll_celery_task(web_url: str, admin_token: str):
    """
    Handle poll_celery script. Steps:
    1. Parse `task_id` arg.
    2. Poll for success of `task_id` (see `poll_celery_task_id`)

    Args:
        web_url: URL to run script on
        admin_token: Needed to authorize use of the update_region endpoint.
    """
    poll_celery_parser = argparse.ArgumentParser(description="Run a login parser.")
    poll_celery_parser.add_argument(
        "--task_id", type=str, required=True, help="Celery task id to poll."
    )
    args, _ = poll_celery_parser.parse_known_args()

    poll_celery_task(web_url, admin_token, args.task_id)


def handle_start_maintenance(web_url: str, admin_token: str):
    """
    Handle start_maintenance script. Steps:
    1. Hit start_maintenance endpoint and check if succeeded
    2. Repeat 1 a max of 450 times with 2 second sleep.

    Args:
        web_url: URL to run script on
        admin_token: Needed to authorize use of the update_region endpoint.
    """
    # 450 * 2 sec = 900 sec = 15 min
    # this can take some time as we wait for problematic tasks to finish.
    num_tries = 450
    sleep_time = 2
    endpoint = "/aws_container/start_maintenance"

    success = False
    for _ in range(num_tries):
        resp = make_post_request(web_url, endpoint, payload=None, admin_token=admin_token)
        assert resp.status_code == 200, f"got code: {resp.status_code}, content: {resp.content}"
        resp_json = resp.json()
        success = resp_json["success"]
        if success:
            break
        else:
            time.sleep(sleep_time)
    assert success, "Failed to start webserver maintenance mode"


def handle_end_maintenance(web_url: str, admin_token: str):
    """
    Handle end_maintenance script. Steps:
    1. Hit end_maintenance endpoint and check if succeeded
    2. Repeat 1 a max of 60 times with 1 second sleep.

    Args:
        web_url: URL to run script on
        admin_token: Needed to authorize use of the update_region endpoint.
    """
    # 60 * 1 sec = 60 sec = 1 min
    # this should run quickly since webserver is not running any problematic tasks
    # on high-load we might fail lock-contention a few times, hence we try for a minute
    num_tries = 60
    sleep_time = 1
    endpoint = "/aws_container/end_maintenance"

    success = False
    for _ in range(num_tries):
        resp = make_post_request(web_url, endpoint, payload=None, admin_token=admin_token)
        assert resp.status_code == 200, f"got code: {resp.status_code}, content: {resp.content}"
        resp_json = resp.json()
        success = resp_json["success"]
        if success:
            break
        else:
            time.sleep(sleep_time)
    assert success, "Failed to end webserver maintenance mode"


def handle_update_region(web_url: str, admin_token: str):
    """
    Handle update_region script. Steps:
    1. Parse `region_name` and `ami` args.
    2. Call `update_region`
    3. Poll for success
    4. Poll subtasks (`update_cluster`) for success
    5. Return nothing iff nothing went wrong. Otherwise errors out.

    Args:
        web_url: URL to run script on
        admin_token: Needed to authorize use of the update_region endpoint.
    """
    update_region_parser = argparse.ArgumentParser(description="Run an update_region parser.")
    update_region_parser.add_argument(
        "--region_name", type=str, required=True, help="Region to update."
    )
    update_region_parser.add_argument("--ami", type=str, required=True, help="New AMI for region.")
    args, _ = update_region_parser.parse_known_args()

    payload = {"region_name": args.region_name, "ami": args.ami}
    endpoint = "/aws_container/update_region"
    resp = make_post_request(web_url, endpoint, payload=payload, admin_token=admin_token)
    assert resp.status_code == 202, f"got code: {resp.status_code}, content: {resp.content}"
    resp_json = resp.json()
    task_id = resp_json["ID"]
    # will error out if task failed
    output = poll_celery_task(web_url, task_id, admin_token)
    subtasks = output["tasks"]
    if len(subtasks) == 0:
        # no subtasks
        return
    # turn subtasks into a list
    # TODO: just dump a list in `update_region` and parse here.
    # This is for backcompat with old bash scripts.
    subtasks = subtasks.split(" ")
    for subtask in subtasks:
        # will error out if task failed
        poll_celery_task(web_url, admin_token, subtask)


def run_script(web_url: str, script: str, admin_token: str = None):
    """
    Dispatch to a script handler based on `script`.

    Args:
        web_url: URL to run script on
        script: Script to run. Must be in SUPPORTED_SCRIPTS
        admin_token: Optionally provided admin token. Some need it (see SCRIPTS_NEEDING_ADMIN)
    """
    assert script in SUPPORTED_SCRIPTS, f"You are trying to run an unsupported script: {script}"
    if admin_token is None:
        # if no admin token, make sure the script does not need an admin token
        assert script not in SCRIPTS_NEEDING_ADMIN, f"Script {script} needs an admin_token"

    # dispatch based on method
    if script == "login":
        return handle_login(web_url)
    elif script == "poll_celery_task":
        return handle_poll_celery_task(web_url, admin_token)
    elif script == "start_maintenance":
        return handle_start_maintenance(web_url, admin_token)
    elif script == "end_maintenance":
        return handle_end_maintenance(web_url, admin_token)
    elif script == "update_region":
        return handle_update_region(web_url, admin_token)
    else:
        raise ValueError(f"Unknown script {script}")


if __name__ == "__main__":
    # generic arguments, each specific script might parse more
    parser = argparse.ArgumentParser(description="Run a supported webserver script.")

    parser.add_argument("--web_url", type=str, required=True, help="URL of webserver to hit.")
    parser.add_argument(
        "--script",
        type=str,
        required=True,
        help=f"Method to invoke. Supported choices: {SUPPORTED_SCRIPTS}",
    )
    parser.add_argument(
        "--admin_token",
        type=str,
        default=None,
        required=False,
        help="Admin token for special permissions.",
    )

    args, _ = parser.parse_known_args()
    run_script(args.web_url, args.script, args.admin_token)
