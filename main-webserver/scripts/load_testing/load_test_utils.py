import time
from typing import List, Tuple

import requests

MAX_POLL_ITERATIONS = 1200


def poll_task_id(task_id: str, web_url: str) -> Tuple[bool, List[int], List[float]]:
    """
    Poll celery task `task_id` for up to 10 minutes. This is an unfortunate duplication of
    `scripts/celery_scripts.py:poll_celery_task` because everything in the `load_test`
    package needs to be runnable in isolation. This is because we package it as a lambda
    function for distributed load testing.

    Args:
        web_url: URL to run script on
        task_id: Task to poll
        admin_token: Optional; can provide traceback info.

    Returns:
        task_output. True iff task succeeded.
        list of status codes received while polling
        list of web response times measured while polling
    """
    status_codes = []
    web_times = []
    success = False
    # 1200 sec * 0.5 sec sleep time = 600 sec = 10 min of polling
    for _ in range(MAX_POLL_ITERATIONS):
        url = f"{web_url}/status/{task_id}"

        start_req = time.time()
        resp = requests.get(
            url=url,
        )
        end_req = time.time()
        web_times.append(end_req - start_req)
        status_codes.append(resp.status_code)
        if resp.status_code == 200:
            resp_json = resp.json()
            if resp_json["state"] == "SUCCESS":
                success = True
                break
            elif resp_json["state"] in ("FAILURE", "REVOKED"):
                raise ValueError(f"Task failed with output: {resp_json['output']}")
            else:
                # request again in 0.5
                time.sleep(0.5)
        elif resp.status_code == 503:
            # request again in 0.5
            time.sleep(0.5)
        elif resp.status_code == 400:
            resp_json = resp.json()
            raise ValueError(f"Task failed with output: {resp_json['output']}")
        else:
            raise ValueError(
                f"Unexpected exit code {resp.status_code}. Response content: {resp.content}."
            )
    return success, status_codes, web_times
