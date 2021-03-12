import time
import argparse

from scripts.celery_scripts import poll_celery_task
from scripts.utils import make_post_request

# all currently supported scripts
SUPPORTED_SCRIPTS = [
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


def start_maintenance(web_url: str, admin_token: str):
    """
    Handle start_maintenance script. Steps:
    1. Hit start_maintenance endpoint and check if succeeded
    2. Repeat 1 a max of 450 times with 2 second sleep.

    Args:
        web_url: URL to run script on
        admin_token: Needed to authorize use of the update_region endpoint.

    Returns:
        None. Errors out on failure.
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


def end_maintenance(web_url: str, admin_token: str):
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


def update_region(web_url: str, admin_token: str, region_name: str, ami: str):
    """
    Run update_region on webserver. Steps:
    1. Call `update_region` endpoint
    2. Poll for success
    3. Poll subtasks (`update_cluster`) for success
    4. Return nothing iff nothing went wrong. Otherwise errors out.

    Args:
        web_url: URL to run script on
        admin_token: Needed to authorize use of the update_region endpoint.
        region_name: Region to update. Ex: us-east-1
        ami: New AMI for region.
    
    Returns:
        None. Errors out if failure.
    """
    payload = {"region_name": region_name, "ami": ami}
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


def handle_start_maintenance(web_url: str, admin_token: str):
    """
    Just a wrapper around `start_maintenance(...)` because no extra args need to be parsed.
    """
    start_maintenance(web_url, admin_token)


def handle_end_maintenance(web_url: str, admin_token: str):
    """
    Just a wrapper around `end_maintenance(...)` because no extra args need to be parsed.
    """
    end_maintenance(web_url, admin_token)


def handle_update_region(web_url: str, admin_token: str):
    """
    Handle update_region script. Steps:
    1. Parse `region_name` and `ami` args.
    2. Call `update_region(...)`
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
    update_region(web_url, admin_token, args.region_name, args.ami)


def run_script(web_url: str, script: str, admin_token: str = None):
    """
    Dispatch to a script handler based on `script`.

    Args:
        web_url: URL to run script on
        script: Script to run. Must be in SUPPORTED_SCRIPTS
        admin_token: Optionally provided admin token. Some need it (see SCRIPTS_NEEDING_ADMIN)
    """
    # dispatch based on method
    if script == "start_maintenance":
        return handle_start_maintenance(web_url, admin_token)
    elif script == "end_maintenance":
        return handle_end_maintenance(web_url, admin_token)
    elif script == "update_region":
        return handle_update_region(web_url, admin_token)
    else:
        raise ValueError(f"Unknown script {script}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run a supported webserver script.")
    parser.add_argument("--web_url", type=str, required=True, help="URL of webserver to hit.")
    parser.add_argument(
        "--script",
        type=str,
        required=True,
        help=f"Script to invoke. Supported choices: {SUPPORTED_SCRIPTS}",
    )
    parser.add_argument(
        "--admin_token",
        type=str,
        default=None,
        required=False,
        help="Admin token for special permissions.",
    )
    args, _ = parser.parse_known_args()
    run_script(args.web_url, args.script)
