import sys
import os
import subprocess
import argparse
import json
import shutil

# this adds the load_testing folder oot to the python path no matter where
# this file is called from. We can now import from `scripts`.
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__)))

from load_test import OUTFOLDER


def invoke_load_test_lambda_func(web_url: str, admin_token: str, func_num: int):
    """

    Args:
        web_url: URL of the webserver to load test
        admin_token: needed to the scripts perform admin-level actions on the webserver.
            See load_test.py:make_request for a full explanation.
        func_num: unique identifier for this lambda function invocation. See
            lambda_function.py:lambda_handler for details.
    """
    savepath = os.path.join(OUTFOLDER, f"out_{func_num}.json")
    payload = {
        "NUM_PROCS": 2,
        "NUM_REQS": 2,
        "WEB_URL": web_url,
        "ADMIN_TOKEN": admin_token,
        "FUNC_NUM": func_num,
    }
    payload_str = json.dumps(payload)
    payload_str = f"'{payload_str}'"  # aws cli expects a '{json...}' around the json
    cmd = f"aws lambda invoke --function-name LoadTest --payload {payload_str} --cli-binary-format raw-in-base64-out {savepath}"
    child = subprocess.Popen(
        cmd,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    return child


def run_distributed_load_test(num_invocations: int, web_url: str, admin_token: str):
    """
    Run a distributed load test using AWS lambda functions and the AWS CLI.
    See invoke_load_test_lambda_func for details on running the lambda function.

    Args:
        num_invocations: number of invocations to invoke_load_test_lambda_func
        web_url: URL of the webserver to load test
        admin_token: needed to the scripts perform admin-level actions on the webserver.
            See load_test.py:make_request for a full explanation.

    Returns:
        Nothing if success. Errors out if a child experiences an error.
    """
    children = []
    for i in range(num_invocations):
        child = invoke_load_test_lambda_func(web_url, admin_token, i)
        children.append(child)
    print("Started all children. Waiting for them to finish...")
    for child in children:
        child.wait()
        child_stdout, child_stderr = child.communicate()
        assert (
            child.returncode == 0
        ), f"Child failed. Child stdout: {child_stdout}, child stderr: {child_stderr}"
    print("Successfully ran distributed load test.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Analyze a load test.")

    parser.add_argument(
        "--num_invocations", type=int, required=True, help="Number of invocations to AWS lambda."
    )
    parser.add_argument("--web_url", type=str, required=True, help="Webserver URL to query.")
    parser.add_argument("--admin_token", type=str, required=True, help="Admin token.")
    args = parser.parse_args()

    if os.path.exists(OUTFOLDER):
        print(f"Cleaning existing {OUTFOLDER}")
        shutil.rmtree(OUTFOLDER)
    os.makedirs(OUTFOLDER, exist_ok=False)

    run_distributed_load_test(args.num_invocations, args.web_url, args.admin_token)
