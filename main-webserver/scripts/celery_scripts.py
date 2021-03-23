import time
import sys
import os
from typing import List, Tuple, Dict

# this adds the webserver repo root to the python path no matter where
# this file is called from. We can now import from `scripts`.
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), ".."))

from scripts.utils import make_get_request


def poll_celery_task(web_url: str, task_id: str, admin_token: str = None):
    """
    Poll celery task `task_id` for up to 10 minutes.
    Args:
        web_url: URL of webserver instance to run operation on
        task_id: Task to poll
        admin_token: Optional; can provide traceback info.

    Returns:
        Errors out if anything goes wrong. Otherwise:
        - the task output
        - list of status codes received while polling
        - list of web response times measured while polling
    """
    print(f"Polling task {task_id}")
    # 300 * 2 sec = 600 sec = 10 min
    num_tries = 300
    sleep_time = 2
    endpoint = f"/status/{task_id}"
    resp_json = None
    for _ in range(num_tries):
        start_req = time.time()
        resp = make_get_request(web_url, endpoint, admin_token=admin_token)
        end_req = time.time()
        # store request data
        status_codes.append(resp.status_code)
        web_times.append(end_req - start_req)
        if resp.status_code == 200:
            resp_json = resp.json()
            if resp_json["state"] == "SUCCESS":
                break
            elif resp_json["state"] in ("FAILURE", "REVOKED"):
                raise ValueError(f"Task failed with output: {resp_json['output']}")
            else:
                time.sleep(sleep_time)
        elif resp.status_code == 503:
            # overloaded server
            time.sleep(sleep_time)
        elif resp.status_code == 400:
            resp_json = resp.json()
            raise ValueError(f"Task failed with output: {resp_json['output']}")
        else:
            # non-pending/started states should exit
            break
    assert resp_json is not None, f"No JSON response for task {task_id}"
    print(f"Got JSON: {resp_json}")
    assert resp_json["state"] == "SUCCESS", f"Got response: {resp_json} for task {task_id}"
    return resp_json["output"]
