import argparse
import json
import multiprocessing as mp
import time
import os

import requests

from load_test_utils import poll_task_id
from load_test_setup import (
    LOAD_TEST_CLUSTER_NAME,
    LOAD_TEST_CLUSTER_REGION,
    LOAD_TEST_USER_PREFIX,
)


def make_request(web_url, admin_token, username, cluster, region, status_codes, web_times):
    url = f"{web_url}/aws_container/assign_container"
    payload = {
        "task_definition_arn": "fractal-dev-browsers-chrome",
        "cluster_name": cluster,
        "username": username,
        "region_name": region,
    }
    headers = {"Authorization": f"Bearer {admin_token}"}
    payload_str = json.dumps(payload)

    success = False
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

    if success is False:
        return None
    return resp.json()["ID"]


def load_test_single_user(web_url, admin_token, user_num):
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
        task_id = make_request(
            web_url,
            admin_token,
            LOAD_TEST_USER_PREFIX.format(user_num=user_num),
            LOAD_TEST_CLUSTER_NAME,
            LOAD_TEST_CLUSTER_REGION,
            status_codes,
            web_times,
        )
        end = time.time()
        request_container_time = end - start
    except Exception as e:
        errors.append(f"Error in making request to assign container. Message: {str(e)}")

    if task_id is not None:
        try:
            start = time.time()
            success = poll_task_id(task_id, web_url, status_codes, web_times)
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


def run_proc(conn, num_reqs, web_url, admin_token, base_user_num):
    responses = []
    for i in range(num_reqs):
        resp = load_test_single_user(web_url, admin_token, base_user_num + i)
        responses.append(resp)
    conn.send(responses)


def multicore_load_test(num_procs, num_reqs, web_url, admin_token, base_user_num):
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

    # this variable is important in distributed contexts
    base_user_num = 0
    response = multicore_load_test(num_procs, num_reqs, web_url, admin_token, base_user_num)

    if not os.path.exists(outfile):
        os.makedirs(outfile)
    fp = open(outfile, "w")
    response_str = json.dumps(response, indent=4)
    fp.write(response_str)
    fp.close()
