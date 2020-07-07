import time
import requests
import progressbar

from tests.constants.heroku import *


def printProgressBar(
    iteration,
    total,
    prefix="",
    suffix="",
    decimals=1,
    length=100,
    fill="",
    printEnd="\r",
):
    bar = progressbar.ProgressBar(
        maxval=20,
        widgets=[progressbar.Bar("=", "[", "]"), " ", progressbar.Percentage()],
    )
    bar.start()
    bar.finish()


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

    if seconds_elapsed > total_timeout_seconds or status != "SUCCESS":
        return -1
    else:
        return 1
