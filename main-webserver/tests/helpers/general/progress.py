import time
import threading
import requests
import progressbar
import multiprocessing

from tests.constants.resources import *
from tests.constants.settings import *


def queryStatus(resp, timeout=10):
    """
    Call in a loop to create terminal progress bar
    @params:
        status_id   - Required  : Celery task ID, returned from function call
        timeout     - Required  : Timeout in minutes, return -1 if timeout is succeeded
    """

    def getStatus(status_id):
        resp = requests.get((SERVER_URL + "/status/" + status_id))
        return resp.json()

    try:
        status_id = resp.json()["ID"]
    except Exception as e:
        print(str(e))
        return -1

    total_timeout_seconds = timeout * 60
    seconds_elapsed = 0

    # Create progress bar

    bar = progressbar.ProgressBar(
        maxval=total_timeout_seconds,
        widgets=[progressbar.Bar("=", "[", "]"), " ", progressbar.Percentage()],
    )
    bar.start()

    status = "PENDING"
    returned_json = None

    # Wait for job to finish

    while (
        status == "PENDING"
        or status == "STARTED"
        and seconds_elapsed < total_timeout_seconds
    ):
        returned_json = getStatus(status_id)
        status = returned_json["state"]

        time.sleep(10)
        seconds_elapsed = seconds_elapsed + 10
        bar.update(seconds_elapsed)

    bar.finish()

    # Check for success, timeout, or failure

    if seconds_elapsed > total_timeout_seconds:
        return {"status": -1, "output": "Timeout error"}
    elif status != "SUCCESS":
        return {
            "status": -2,
            "output": "Did not receive SUCCESS, instead saw {error}".format(
                error=str(returned_json)
            ),
        }
    else:
        return {"status": 1, "output": "SUCCESS detected"}


def fractalJobRunner(f, initial_list):
    if initial_list:
        if ALLOW_MULTITHREADING:
            thread_tracker = [None] * len(initial_list)

            for i in range(0, len(initial_list)):
                element = initial_list[i]
                thread_tracker[i] = threading.Thread(target=f, args=(element,))
                thread_tracker[i].start()
            for thread in thread_tracker:
                thread.join()
        else:
            for element in initial_list:
                f(element)
