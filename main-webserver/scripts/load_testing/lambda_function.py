"""
For use as an AWS lambda function.
"""

import json
import os
import subprocess

from load_test import multicore_load_test


def lambda_handler(event, context):
    num_procs = int(event["NUM_PROCS"])
    num_reqs = int(event["NUM_REQS"])
    web_url = event["WEB_URL"]
    admin_token = event["ADMIN_TOKEN"]
    # this should be provided in the AWS lambda invocation
    func_num = int(event["FUNC_NUM"])
    # each lambda function handles `num_reqs` reqs
    # to properly distribute the test users, we compute a base_user_num
    # each lambda function uses the test users from [`base_user_num`, `base_user_num + num_reqs - 1`]
    base_user_num = func_num * num_reqs
    output = multicore_load_test(num_procs, num_reqs, web_url, admin_token, base_user_num)
    return {"statusCode": 200, "body": json.dumps(output)}
