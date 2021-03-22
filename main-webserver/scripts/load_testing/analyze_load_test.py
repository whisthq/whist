import sys
import os
import json

# this adds the webserver repo root to the python path no matter where
# this file is called from. We can now import from `scripts`.
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "../.."))

from scripts.load_testing.analyze_load_test import OUTFOLDER


def load_load_test_data(fp):
    # TODO: port for distributed
    return json.load(fp)


def analyze_load_test():
    assert os.path.isdir(OUTFOLDER)
    files = os.listdir(OUTFOLDER)
    all_load_test_data = []
    for f in files:
        with open(os.path.join(OUTFOLDER, f)) as fp:
            # data will be a list of JSON outputs
            data = load_load_test_data(fp)
            all_load_test_data += data

    # scan for errors
    user_had_error = False
    for data in all_load_test_data:
        if len(data["errors"]) > 0:
            user_had_error = True
            task_id = data["task_id"]
            errors = data["errors"]
            print(f"Task {task_id} experienced errors: {errors}")
    assert user_had_error is False

    # do analysis if no errors
    all_status_codes = []
    all_web_times = []
    request_container_times = []
    poll_container_times = []
    for data in all_load_test_data:
        all_status_codes += data["status_codes"]
        all_web_times += data["web_times"]
        request_container_times.append(data["request_container_time"])
        pct = data["poll_container_time"]
        if pct is not None:
            poll_container_times.append(pct)


if __name__ == "__main__":
    analyze_load_test()
