"""
Run a load test. Provides a neat wrapper over running the load test with locust
and setting up the inputs.
"""

import sys
import os
import subprocess
import shutil

# this adds the load_testing folder oot to the python path no matter where
# this file is called from. We can now import from `scripts`.
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "../.."))

from scripts.load_testing.load_test_utils import OUTFOLDER, CSV_PREFIX


def run_local_load_test(web_url: str, admin_token: str, num_users: int):
    """
    Run a local load test using locust.

    Args:
        web_url: URL of the webserver to load test
        admin_token: needed to the scripts perform admin-level actions on the webserver.
            See load_test.py:load_test_single_user for a full explanation.
        num_users: total number of users to simulate

    Returns:
        Calls `locust` utility on `locust_load_test.py`. It will exit with nonzero exit
        code if any user cannot get a container. If that happens, this function will
        also exit with error.
    """
    if os.path.exists(OUTFOLDER):
        print(f"Cleaning existing {OUTFOLDER}")
        shutil.rmtree(OUTFOLDER)
    os.makedirs(OUTFOLDER, exist_ok=False)

    csv_path = os.path.join(OUTFOLDER, CSV_PREFIX)
    # add admin to environment; load test script will read it
    os.environ["ADMIN_TOKEN"] = admin_token
    # -u: number of users
    # -r: users/s generation rate
    # --host: webserver url
    # --headless: do not start a local web server showing results
    # --csv: path to save results
    # --only-summary: only print result summaries
    cmd = (
        f"locust -f locust_load_test.py -u {num_users} -r 10 --host {web_url} "
        f"--headless --only-summary"
    )
    ret = subprocess.run(cmd, shell=True)
    assert ret.returncode == 0, "Failed to run a local load test."
    print("Successfully ran a local load test.")
