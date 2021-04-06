import time
import sys
import os

# this adds the webserver repo root to the python path no matter where
# this file is called from. We can now import from `scripts`.
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), ".."))

from scripts.celery_scripts import poll_celery_task
from scripts.utils import make_post_request


def start_maintenance(web_url: str, admin_token: str):
    """
    Handle start_maintenance script. Steps:
    1. Hit start_maintenance endpoint and check if succeeded
    2. Repeat 1 a max of 450 times with 2 second sleep.

    Args:
        web_url: URL of webserver instance to run operation on
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
        web_url: URL of webserver instance to run operation on
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
        web_url: URL of webserver instance to run operation on
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
    for subtask_id in subtasks:
        # will error out if task failed
        poll_celery_task(web_url, subtask_id, admin_token)


def update_taskdefs(
    web_url: str, admin_token: str, app_id: str = None, task_definition_arn: str = None
):
    """
    Run update_taskdefs on webserver. Steps:
    1. Call `update_taskdefs` endpoint
    2. Poll for success
    3. Return nothing iff nothing went wrong. Otherwise errors out.

    Args:
        web_url: URL of webserver instance to run operation on
        admin_token: Needed to authorize use of the update_region endpoint.
        app_id: Optional. Update a specific `app_id` to `task_definition_arn`. Otherwise update
            all app_ids in db to their latest version according to AWS.
        task_definition_arn: Optional. See `app_id` docstring.

    Returns:
        None. Errors out if failure.
    """
    payload = None
    if app_id is not None:
        assert task_definition_arn is not None
        payload = {"app_id": app_id, "task_definition_arn": task_definition_arn}
    endpoint = "/aws_container/update_taskdefs"
    resp = make_post_request(web_url, endpoint, payload=payload, admin_token=admin_token)
    assert resp.status_code == 202, f"got code: {resp.status_code}, content: {resp.content}"
    resp_json = resp.json()
    task_id = resp_json["ID"]
    # will error out if task failed
    poll_celery_task(web_url, task_id, admin_token)
