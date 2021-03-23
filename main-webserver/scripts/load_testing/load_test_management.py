"""
Setup a load test. Supports the following methods:
- create_cluster with the name LOAD_TEST_CLUSTER_NAME
- delete_cluster with the name LOAD_TEST_CLUSTER_NAME
- make_load_test_user
- upgrade_webserver
- downgrade_webserver
"""
import sys
import os
import json
import argparse
import subprocess

# this adds the load_testing folder to the python path no matter where
# this file is called from. We can now import from `scripts`.
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__)))

import requests

# locally, this is imported from the `scripts` in monorepo
# in the AWS lambda function, this is imported from the symlinked `scripts`
# in the load_testing folder
from scripts.celery_scripts import poll_celery_task
from scripts.utils import make_post_request
from load_test_core import (
    LOAD_TEST_CLUSTER_NAME,
    LOAD_TEST_CLUSTER_REGION,
    LOAD_TEST_USER_PREFIX,
)


def create_load_test_cluster(web_url: str, admin_token: str):
    """
    Make the load testing cluster

    Args:
        web_url: URL of the webserver to load test
        admin_token: needed to make the cluster

    Returns:
        None. Errors out if anything goes wrong.
    """
    endpoint = "/aws_container/create_cluster"
    payload = {
        "cluster_name": LOAD_TEST_CLUSTER_NAME,
        "instance_type": "g3.4xlarge",
        "region_name": LOAD_TEST_CLUSTER_REGION,
        "max_size": 10,
        "min_size": 1,
    }
    resp = make_post_request(web_url, endpoint, payload=payload, admin_token=admin_token)
    assert resp.status_code == 202, f"Got bad response code {resp.status_code}, msg: {resp.content}"

    task_id = resp.json()["ID"]

    # errors out on failure
    poll_celery_task(web_url, task_id, admin_token)


def delete_load_test_cluster(web_url: str, admin_token: str):
    """
    Delete the load testing cluster.

    Args:
        web_url: URL of the webserver to load test
        admin_token: needed to delete the cluster

    Returns:
        None. Errors out if anything goes wrong.
    """
    endpoint = "/aws_container/delete_cluster"
    payload = {
        "cluster_name": LOAD_TEST_CLUSTER_NAME,
        "region_name": LOAD_TEST_CLUSTER_REGION,
    }
    resp = make_post_request(web_url, endpoint, payload=payload, admin_token=admin_token)
    assert resp.status_code == 202, f"Got bad response code {resp.status_code}, msg: {resp.content}"

    task_id = resp.json()["ID"]

    # errors out on failure
    poll_celery_task(task_id, web_url)


def upgrade_webserver(app_name):
    """
    Upgrade webserver to be prod-like. Specifically, it should run a Performance-M web dyno with
    autoscaling enabled and a 500ms desired response time.

    Args:
        app_name: Name of app of webserver to scale.

    Returns:
        None. Errors out on any failure.
    """
    ret = subprocess.run(f"heroku ps:scale web=1:Performance-M --app {app_name}", shell=True)
    assert ret.returncode == 0
    ret = subprocess.run(
        f"heroku ps:autoscale:enable --min 1 --max 10 --p95 500 --app {app_name}", shell=True
    )
    assert ret.returncode == 0


def downgrade_webserver(app_name):
    """
    Undo changes in upgrade_webserver.

    Args:
        app_name: Name of app of webserver to scale.

    Returns:
        None. Errors out on any failure.
    """
    ret = subprocess.run(f"heroku ps:scale web=1:Standard-1X --app {app_name}", shell=True)
    assert ret.returncode == 0
    ret = subprocess.run(f"heroku ps:autoscale:disable --app {app_name}", shell=True)
    assert ret.returncode == 0


def make_load_test_user(web_url: str, admin_token: str, user_num: int):
    """
    Create a user with the name LOAD_TEST_USER_PREFIX.format(user_num=i) for load testing

    Args:
        web_url: URL of the webserver to load test
        admin_token: needed to delete the cluster
        username: name of the user to create

    Returns:
        None. Errors out if anything goes wrong.
    """
    username = LOAD_TEST_USER_PREFIX.format(user_num=i)
    endpoint = "/account/register"
    payload = {"username": username, "password": username, "name": username, "feedback": username}
    resp = make_post_request(web_url, endpoint, payload=payload, admin_token=admin_token)
    assert resp.status_code == 200, f"Got bad response code {resp.status_code}, msg: {resp.content}"
