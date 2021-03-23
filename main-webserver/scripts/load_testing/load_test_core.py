"""
This file contains the code needed to load test on a single machine, 
either in local or in distributed (AWS lambda) contexts.
"""

import sys
import os
import argparse
import json
import multiprocessing as mp
from multiprocessing.connection import Connection
import time
import shutil
from typing import Tuple, List, Dict

# this adds the load_testing folder oot to the python path no matter where
# this file is called from. We can now import from `scripts`.
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__)))

import requests

# locally, this is imported from the `scripts` in monorepo
# in the AWS lambda function, this is imported from the symlinked `scripts`
# in the load_testing folder
from scripts.celery_scripts import poll_celery_task
from scripts.utils import make_post_request

LOAD_TEST_CLUSTER_NAME = "load_test_cluster"
LOAD_TEST_CLUSTER_REGION = "us-east-1"
LOAD_TEST_USER_PREFIX = "load_test_user_{user_num}"

OUTFOLDER = "load_test_dump"


def make_assign_request(
    web_url: str,
    admin_token: str,
    user_num: int,
    num_tries: int = 100,
    sleep_time: float = 0.5,
) -> Tuple[str, List[int], List[float]]:
    """
    Request a container and get a task to poll for success.

    Args:
        web_url: URL of the webserver to load test
        admin_token: needed to hit this endpoint. We can't use /container/assign
            because we need to chose the load test cluster `cluster`.
        username: user to make the request as
        num_tries: number of times to try the assign request
        sleep_time: sleep time in between assign requests

    Returns:
        Errors out if something unexpected occurs. Otherwise:
        (celery task id,
        list of status codes received while polling,
        list of web response times measured while polling
        )
    """
    endpoint = "/aws_container/assign_container"
    payload = {
        "task_definition_arn": "fractal-staging-browsers-chrome",
        "cluster_name": LOAD_TEST_CLUSTER_NAME,
        "username": LOAD_TEST_USER_PREFIX.format(user_num=user_num),
        "region_name": LOAD_TEST_CLUSTER_REGION,
    }

    success = False
    web_times = []
    status_codes = []
    for _ in range(num_tries):
        start = time.time()
        resp = make_post_request(web_url, endpoint, payload=payload, admin_token=admin_token)
        end = time.time()

        status_codes.append(resp.status_code)
        web_times.append(end - start)
        if resp.status_code == 202:
            success = True
            break
        elif resp.status_code == 503:
            # overloaded server; sleep and try again
            time.sleep(sleep_time)
        else:
            # any other status code is unexpected; error out
            raise ValueError(
                f"Unexpected exit code {resp.status_code}. Response content: {resp.content}."
            )

    assert success is True

    return resp.json()["ID"], status_codes, web_times


def load_test_single_user(web_url: str, admin_token: str, user_num: int):
    """
    Args:
        web_url: URL of the webserver to load test
        admin_token: needed to request a container in the load test cluster.
        user_num: which load test user to use

    Returns:
        A dict:
            {
                "task_id": str,
                "errors": [...],
                "status_codes": [...],
                "web_times": [...],
                "poll_container_time": float,
            }
    """
    errors = []
    status_codes = []
    web_times = []
    task_id = None
    poll_container_time = None
    try:
        task_id, new_status_codes, new_web_times = make_assign_request(
            web_url, admin_token, user_num
        )
        status_codes += new_status_codes
        web_times += new_web_times
    except Exception as e:
        errors.append(f"Error in making request to assign container. Message: {str(e)}")

    if task_id is not None:
        try:
            start = time.time()
            # num_tries = 1200, sleep_time = 0.5 -> 600 sec -> 10 min
            # errors out on failure
            # the first return would tell us how to connect to our container, but for load testing
            # we stop here
            _, new_status_codes, new_web_times = poll_celery_task(
                web_url, task_id, admin_token=admin_token, num_tries=1200, sleep_time=0.5
            )
            end = time.time()

            status_codes += new_status_codes
            web_times += new_web_times
            poll_container_time = end - start
        except Exception as e:
            errors.append(f"Error in polling status of assign container. Message: {str(e)}")

    return {
        "task_id": task_id,
        "errors": errors,
        "status_codes": status_codes,
        "web_times": web_times,
        "poll_container_time": poll_container_time,
    }


def run_proc(
    conn: Connection,
    num_reqs: int,
    web_url: str,
    admin_token: str,
    base_user_num: int,
):
    """
    Run a single process for load testing.

    Args:
        conn: connection to parent for communicating results
        num_reqs: total number of requests to make
        web_url: URL of the webserver to load test
        admin_token: needed to the scripts perform admin-level actions on the webserver.
            See load_test.py:load_test_single_user for a full explanation.
        base_user_num: this process will use users in
            [base_user_num, base_user_num + num_reqs - 1]

    Returns:
        None. Sends a list of the returns from load_test_single_user to the main process.
    """
    responses = []
    for i in range(num_reqs):
        resp = load_test_single_user(web_url, admin_token, base_user_num + i)
        responses.append(resp)
    conn.send(responses)


def multicore_load_test(
    num_procs: int, num_reqs: int, web_url: str, admin_token: str, base_user_num: int
) -> List[Dict]:
    """
    Calls run_proc `num_procs` times.

    Args:
        num_procs: total number of process to create
        num_reqs: total number of requests to make
        web_url: URL of the webserver to load test
        admin_token: needed to the scripts perform admin-level actions on the webserver.
            See load_test.py:load_test_single_user for a full explanation.
        base_user_num: this multicore load test will use users in
            [base_user_num, base_user_num + num_reqs - 1]

    Returns:
        A list of the returns from load_test_single_user aggregated from the child processes.
    """
    reqs_per_proc = num_reqs // num_procs
    reqs_first_proc = num_reqs - (num_procs - 1) * reqs_per_proc

    procs = []
    conns = []
    for i in range(num_procs):
        parent_conn, child_conn = mp.Pipe()
        if i == 0:
            proc = mp.Process(
                target=run_proc,
                args=(
                    child_conn,
                    reqs_first_proc,
                    web_url,
                    admin_token,
                    base_user_num,
                ),
            )
            # increment this number so next process uses unique set of users
            base_user_num += reqs_first_proc
        else:
            proc = mp.Process(
                target=run_proc,
                args=(
                    child_conn,
                    reqs_per_proc,
                    web_url,
                    admin_token,
                    base_user_num,
                ),
            )
            # increment this number so next process uses unique set of users
            base_user_num += reqs_per_proc
        proc.start()
        procs.append(proc)
        conns.append(parent_conn)

    for i in range(num_procs):
        procs[i].join()

    responses = []  # flat list of all responses from load_test_single_user
    for i in range(num_procs):
        resp = conns[i].recv()
        responses += resp
    return responses
