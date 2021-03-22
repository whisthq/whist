import sys
import os
import argparse
import json
import multiprocessing as mp
import time
import shutil
from typing import Tuple, List, Dict

# this adds the load_testing folder oot to the python path no matter where
# this file is called from. We can now import from `scripts`.
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__)))

import requests

from load_test_utils import poll_task_id
from load_test_setup import (
    LOAD_TEST_CLUSTER_NAME,
    LOAD_TEST_CLUSTER_REGION,
    LOAD_TEST_USER_PREFIX,
)

OUTFOLDER = "load_test_dump"


def make_request(
    web_url: str, admin_token: str, username: str, cluster: str, region: str
) -> Tuple[str, List[int], List[float]]:
    """
    Request a container and get a task to poll for success.

    Args:
        web_url: URL of the webserver to load test
        admin_token: needed to hit this endpoint. We can't use /container/assign
            because we need to chose the load test cluster `cluster`.
        username: user to make the request as
        cluster: cluster to get the container in
        region: region to get the container in

    Returns:
        celery task id on success.
        list of status codes received while polling
        list of web response times measured while polling
    """
    url = f"{web_url}/aws_container/assign_container"
    payload = {
        "task_definition_arn": "fractal-staging-browsers-chrome",
        "cluster_name": cluster,
        "username": username,
        "region_name": region,
    }
    headers = {"Authorization": f"Bearer {admin_token}"}
    payload_str = json.dumps(payload)

    success = False

    web_times = []
    status_codes = []
    for _ in range(100):
        start_req = time.time()
        resp = requests.post(url=url, headers=headers, data=payload_str)
        end_req = time.time()
        web_times.append(end_req - start_req)
        status_codes.append(resp.status_code)
        if resp.status_code == 202:
            success = True
            break
        else:
            time.sleep(0.5)

    assert success is True

    return resp.json()["ID"], status_codes, web_times


def load_test_single_user(web_url: str, admin_token: str, user_num: int):
    """
    Args:
        web_url: URL of the webserver to load test
        admin_token: needed to the scripts perform admin-level actions on the webserver.
            See load_test.py:make_request for a full explanation.
        user_num: which load test user to use

    Returns:
        A dict:
            {
                "task_id": str,
                "errors": [...],
                "status_codes": [...],
                "web_times": [...],
                "request_container_time": float,
                "poll_container_time": float,
            }
    """
    if web_url[-1] == "/":
        web_url = web_url[:-1]

    errors = []
    status_codes = []
    web_times = []
    task_id = None
    request_container_time = None
    poll_container_time = None
    try:
        start = time.time()
        task_id, new_status_codes, new_web_times = make_request(
            web_url,
            admin_token,
            LOAD_TEST_USER_PREFIX.format(user_num=user_num),
            LOAD_TEST_CLUSTER_NAME,
            LOAD_TEST_CLUSTER_REGION,
        )
        end = time.time()
        request_container_time = end - start
        status_codes += new_status_codes
        web_times += new_web_times
    except Exception as e:
        errors.append(f"Error in making request to assign container. Message: {str(e)}")

    if task_id is not None:
        try:
            start = time.time()
            success, new_status_codes, new_web_times = poll_task_id(task_id, web_url)
            status_codes += new_status_codes
            web_times += new_web_times
            assert success is True
            end = time.time()
            poll_container_time = end - start
        except Exception as e:
            errors.append(f"Error in polling status of assign container. Message: {str(e)}")

    return {
        "task_id": task_id,
        "errors": errors,
        "status_codes": status_codes,
        "web_times": web_times,
        "request_container_time": request_container_time,
        "poll_container_time": poll_container_time,
    }


def run_proc(
    conn: mp.connection.Connection,
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
            See load_test.py:make_request for a full explanation.
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
            See load_test.py:make_request for a full explanation.
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


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run the load tester.")

    parser.add_argument("--num_procs", type=int, help="Number of processors.", required=True)
    parser.add_argument("--num_reqs", type=int, help="Number of requests.", required=True)
    parser.add_argument("--web_url", type=str, help="Webserver URL to query.", required=True)
    parser.add_argument("--admin_token", type=str, help="Admin token.", required=True)
    parser.add_argument("--outfile", type=str, help="File to test data to.", required=True)

    args = parser.parse_args()

    num_procs = args.num_procs
    num_reqs = args.num_reqs
    web_url = args.web_url
    admin_token = args.admin_token
    outfile = args.outfile

    if os.path.exists(OUTFOLDER):
        print(f"Cleaning existing {OUTFOLDER}")
        shutil.rmtree(OUTFOLDER)
    os.makedirs(OUTFOLDER, exist_ok=False)

    # this variable is important in distributed contexts; it tells an invocation of
    # `multicore_load_test` which user it should start with
    base_user_num = 0
    response = multicore_load_test(num_procs, num_reqs, web_url, admin_token, base_user_num)

    outpath = os.path.join(OUTFOLDER, outfile)
    print(f"Saving to {outpath}")
    fp = open(outpath, "w")
    response_str = json.dumps(response, indent=4)
    fp.write(response_str)
    fp.close()
