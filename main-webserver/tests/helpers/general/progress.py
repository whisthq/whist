import time
import requests

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
    """
    Call in a loop to create terminal progress bar
    @params:
        iteration   - Required  : current iteration (Int)
        total       - Required  : total iterations (Int)
        prefix      - Optional  : prefix string (Str)
        suffix      - Optional  : suffix string (Str)
        decimals    - Optional  : positive number of decimals in percent complete (Int)
        length      - Optional  : character length of bar (Int)
        fill        - Optional  : bar fill character (Str)
        printEnd    - Optional  : end character (e.g. "\r", "\r\n") (Str)
    """
    percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
    filledLength = int(length * iteration // total)
    bar = fill * filledLength + "-" * (length - filledLength)
    print(f"\r{prefix} |{bar}| {percent}% {suffix}", end=printEnd)
    # Print New Line on Complete
    if iteration == total:
        print()


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

    status = "PENDING"
    while (
        status == "PENDING"
        or status == "STARTED"
        and seconds_elapsed < total_timeout_seconds
    ):
        returned_json = getStatus(status_id)
        status = returned_json["state"]

        time.sleep(10)
        seconds_elapsed += 10

        printProgressBar(
            seconds_elpased / 10,
            total_timeout_seconds,
            suffix=str(returned_json["output"]),
        )

    if seconds_elapsed > total_timeout_seconds or status != "SUCCESS":
        return -1
    else:
        return 1
