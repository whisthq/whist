"""
This file lets us run our load tests as AWS lambda functions.
"""

import json
import os
import subprocess

from load_test import multicore_load_test


def lambda_handler(event, context):
    """
    Run the load test as a lambda function.

    Args:
        event: Must contain NUM_PROCS, NUM_REQS, WEB_URL, ADMIN_TOKEN, and FUNC_NUM. These
            are passed by run_distributed_load_test.py:invoke_load_test_lambda_func

            NUM_PROCS: number of processes to run. Stick to 1 or 2 on lambda functions.
            NUM_REQS: number of requests to do (total). This will be split up on the processes.
            WEB_URL: URL of the webserver to load test
            ADMIN_TOKEN: needed to the scripts perform admin-level actions on the webserver.
                See load_test.py:make_request for a full explanation.
            FUNC_NUM: each lambda function handles `num_reqs` reqs. to properly distribute
                the test users, we use this number to figure out which of the load_test_users
                (already inserted in the database) we should make requests as.

    Returns:
        A JSON of {statusCode: int, body: str}
    """
    num_procs = int(event["NUM_PROCS"])
    num_reqs = int(event["NUM_REQS"])
    web_url = event["WEB_URL"]
    admin_token = event["ADMIN_TOKEN"]
    func_num = int(event["FUNC_NUM"])
    # to properly distribute the test users, we compute a base_user_num
    # each lambda function uses the load test users from
    # [`base_user_num`, `base_user_num + num_reqs - 1`]
    base_user_num = func_num * num_reqs
    output = multicore_load_test(num_procs, num_reqs, web_url, admin_token, base_user_num)
    return {"statusCode": 200, "body": json.dumps(output)}
