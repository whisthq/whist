"""
Run a load test. Provides a neat wrapper over running the load test with locust
and setting up the inputs.
"""

import sys
import os
import subprocess
import shutil

# this adds the webserver repo root to the python path no matter where
# this file is called from. We can now import from `scripts`.
webserver_root = os.path.join(os.getcwd(), os.path.dirname(__file__), "../..")
sys.path.append(webserver_root)

def run_local_load_test(web_url: str, admin_token: str, num_users: int, region_name: str):
    """
    Run a local load test using locust.

    Args:
        web_url: URL of the webserver to load test
        admin_token: needed to the scripts perform admin-level actions on the webserver.
            See load_test.py:load_test_single_user for a full explanation.
        num_users: total number of users to simulate
        region_name: region to load test in

    Returns:
        Calls `locust` utility on `locust_load_test.py`. It will exit with nonzero exit
        code if any user cannot get a container. If that happens, this function will
        also exit with error.
    """
    locust_file_path = os.path.join(webserver_root, "scripts", "load_testing", "locust_load_test.py")
    # add admin_token and region_name to environment; load test script will read it
    os.environ["ADMIN_TOKEN"] = admin_token
    os.environ["REGION_NAME"] = region_name
    # -u: number of users
    # -r: users/s generation rate
    # --host: webserver url
    # --headless: do not start a local web server showing results
    # --only-summary: only print result summaries
    cmd = (
        f"locust -f {locust_file_path} -u {num_users} -r 10 --host {web_url} "
        f"--headless --only-summary"
    )
    ret = subprocess.run(cmd, shell=True)
    assert ret.returncode == 0, "Failed to run a local load test."
    print("Successfully ran a local load test.")
