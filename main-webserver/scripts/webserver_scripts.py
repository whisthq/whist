import os
import argparse
import time
import json
from typing import List

import requests


SUPPORTED_SCRIPTS = [
    "login",
    "poll_celery_task",
    "start_maintenance",
    "end_maintenance",
    "update_region",
]

SCRIPTS_NEEDING_ADMIN = [
    "start_maintenance",
    "end_maintenance",
    "update_region",
]


def join_url_endpoint(web_url: str, endpoint: str):
    if web_url[-1] == "/":
        web_url = web_url[:-1]
    if len(endpoint) > 0 and endpoint[0] == "/":
        endpoint = endpoint[1:]
    return f"{web_url}/{endpoint}"


def make_get_request(web_url: str, endpoint: str, admin_token: str = None):
    url = join_url_endpoint(web_url, endpoint)
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
    url = join_url_endpoint(web_url, endpoint)
    headers = None
    if admin_token is not None:
        headers = {
            "Authorization": f"Bearer {admin_token}",
        }
    payload_str = None
    if payload is not None:
        payload_str = json.dumps(payload)
    return requests.post(url=url, headers=headers, data=payload_str)


def handle_login(web_url: str):
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


def poll_celery_task(web_url: str, admin_token: str, task_id: str):
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
    poll_celery_parser = argparse.ArgumentParser(description="Run a login parser.")
    poll_celery_parser.add_argument(
        "--task_id", type=str, required=True, help="Celery task id to poll."
    )
    args, _ = poll_celery_parser.parse_known_args()

    return poll_celery_task(web_url, admin_token, args.task_id)


def handle_start_maintenance(web_url: str, admin_token: str):
    # 300 * 2 sec = 600 sec = 10 min
    num_tries = 300
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
    # 300 * 2 sec = 600 sec = 10 min
    num_tries = 300
    sleep_time = 2
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
    output = poll_celery_task(web_url, admin_token, task_id)
    subtasks = output["tasks"]
    if len(subtasks) == 0:
        # no subtasks
        return
    # turn subtasks into a list
    # TODO: just dump a list in `update_region`. This is for backcompat with old bash scripts.
    subtasks = subtasks.split(" ")
    for subtask in subtasks:
        # will error out if task failed
        poll_celery_task(web_url, admin_token, subtask)


def run_script(web_url: str, method: str, admin_token: str = None):
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
    else:
        raise ValueError(f"Unknown method {method}")


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
