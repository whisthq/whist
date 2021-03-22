import time

import requests


def poll_task_id(task_id, web_url, status_codes, web_times):
    success = False
    for _ in range(200):
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
            elif resp_json["state"] == "FAILURE":
                raise ValueError(f"Task failed with output: {resp_json['output']}")
            else:
                # request again in 0.5
                time.sleep(0.5)
        elif resp.status_code == 400:
            resp_json = resp.json()
            raise ValueError(f"Task failed with output: {resp_json['output']}")
        elif resp.status_code == 503:
            # overloaded server
            time.sleep(1.0)
        else:
            raise ValueError(
                f"Unexpected exit code {resp.status_code}. Response content: {resp.content}."
            )
    return success
