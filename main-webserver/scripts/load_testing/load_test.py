import sys
import os
import subprocess
import argparse
import json
import shutil

# this adds the load_testing folder oot to the python path no matter where
# this file is called from. We can now import from `scripts`.
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__)))

from load_test_core import OUTFOLDER, multicore_load_test


def invoke_load_test_lambda_func(web_url: str, admin_token: str, func_num: int):
    """
    Uses AWS CLI to run a lambda function with specific parameters. See how this
    is handled in lambda_function.py.

    Args:
        web_url: URL of the webserver to load test
        admin_token: needed to the scripts perform admin-level actions on the webserver.
            See load_test.py:load_test_single_user for a full explanation.
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


def run_distributed_load_test(web_url: str, admin_token: str, num_invocations: int):
    """
    Run a distributed load test using AWS lambda functions and the AWS CLI.
    See invoke_load_test_lambda_func for details on running the lambda function.

    Args:
        web_url: URL of the webserver to load test
        admin_token: needed to the scripts perform admin-level actions on the webserver.
            See load_test.py:load_test_single_user for a full explanation.
        num_invocations: number of invocations to invoke_load_test_lambda_func

    Returns:
        Nothing if success. Errors out if a child experiences an error.
    """
    if os.path.exists(OUTFOLDER):
        print(f"Cleaning existing {OUTFOLDER}")
        shutil.rmtree(OUTFOLDER)
    os.makedirs(OUTFOLDER, exist_ok=False)

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


def run_local_load_test(web_url: str, admin_token: str, num_procs: int, num_reqs: int):
    """
    Run a local load test using `multicore_load_test`.

    Args:
        web_url: URL of the webserver to load test
        admin_token: needed to the scripts perform admin-level actions on the webserver.
            See load_test.py:load_test_single_user for a full explanation.
        num_procs: number of processors to run load test with
        num_reqs: total number of requests to make

    Returns:
        Writes results to `OUTFOLDER`/out_local.json.
    """
    if os.path.exists(OUTFOLDER):
        print(f"Cleaning existing {OUTFOLDER}")
        shutil.rmtree(OUTFOLDER)
    os.makedirs(OUTFOLDER, exist_ok=False)

    # this variable is important in distributed contexts; it tells an invocation of
    # `multicore_load_test` which user it should start with. in local we just start at 0.
    base_user_num = 0
    response = multicore_load_test(num_procs, num_reqs, web_url, admin_token, base_user_num)

    outfile = "out_local.json"
    outpath = os.path.join(OUTFOLDER, outfile)

    print(f"Saving local results to {outpath}")
    fp = open(outpath, "w")
    response_str = json.dumps(response, indent=4)
    fp.write(response_str)
    fp.close()

    print("Successfully ran local load test.")
