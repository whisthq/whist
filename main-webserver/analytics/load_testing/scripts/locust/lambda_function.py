"""
For use as an AWS lambda function.
"""

import json
import os
import subprocess


def lambda_handler(event, context):
    # make sure all necessary env vars are defined
    num_reqs = os.environ["NUM_REQS"]
    web_url = os.environ["WEB_URL"]
    # make sure this is defined; load_test.py uses directly
    admin_token = os.environ["ADMIN_TOKEN"]

    output = subprocess.check_output(
        [
            "locust",
            "-f",
            "load_test.py",
            "--headless",
            "-u",
            str(num_reqs),
            "-r",
            "100",
            "--host",
            str(web_url),
            "--only-summary",
        ]
    )

    output_str = output.decode("utf-8")

    return {"statusCode": 200, "body": json.dumps(output_str)}
