"""
File provided to locust CLI to run a load test.
"""


import os
import time
import sys
import json
from functools import wraps
import csv

# this adds the webserver repo root to the python path no matter where
# this file is called from. We can now import from `scripts`.
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "../.."))

import locust

from scripts.load_testing.load_test_utils import (
    LOAD_TEST_CLUSTER_NAME,
    LOAD_TEST_CLUSTER_REGION,
    LOAD_TEST_USER_PREFIX,
    get_task_definition_arn,
    OUTFOLDER,
    CSV_PREFIX,
)


def get_num_users_and_host():
    """
    -u (number of users), --host (webserver url) are passed to locust via CLI args. However, Locust
    does not provide a way to access these variables. So, we extract them ourselves.
    """
    # figured this out by looking at main() in locust source
    options = locust.argument_parser.parse_options()
    num_users = options.num_users
    host = options.host
    if host[-1] == "/":
        # allows users to pass a URL ending with a slash
        # ex: www.google.com/ -> www.google.com
        host = host[:-1]
    return num_users, host


TOTAL_USERS, WEB_URL = get_num_users_and_host()
ADMIN_TOKEN = os.environ["ADMIN_TOKEN"]
CUSTOM_CSV_STATS_PATH = os.path.join(OUTFOLDER, f"{CSV_PREFIX}_user_stats.csv")


def setup_custom_stats():
    with open(CUSTOM_CSV_STATS_PATH, "w") as f:
        csv_writer = csv.writer(f)
        csv_writer.writerow(["user_num", "container_time"])  # custom stats to collect


def save_custom_stats(user_num: int, container_time: float):
    with open(CUSTOM_CSV_STATS_PATH, "a") as f:
        csv_writer = csv.writer(f)
        csv_writer.writerow([user_num, container_time])


setup_custom_stats()


def preprocess_locust_task(func):
    @wraps(func)
    def wrapper(self, *args, **kwargs):
        if self.error is True:
            raise locust.exception.StopUser()
        return func(self, *args, **kwargs)

    return wrapper


class LoadTestUser(locust.HttpUser):
    # this occurs after every retry on a function decorated with @locust.task
    wait_time = locust.between(0.5, 1.0)

    # because each locust invocation is single-threaded and switches between tasks with gevent
    # so we can use variables that otherwise need lock-access.
    # available_user_num: number representing the largest unclaimed user number. on_start(...)
    #   uses this to pick a user to make a request on behalf of.
    # num_finished: number of users that have finished.
    available_user_num = 1
    num_finished = 0

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.error = False  # used by self.raise_fatal_exception, see docstring for details

        # no need for lock, see comments on LoadTestUser.available_user_num
        self.user_num = LoadTestUser.available_user_num
        LoadTestUser.available_user_num += 1

        # task id to poll
        self.task_id = None

        # timing
        self.start_time = None
        self.end_time = None

    def on_start(self):
        url = f"{WEB_URL}/aws_container/assign_container"
        payload = json.dumps(
            {
                "task_definition_arn": get_task_definition_arn(WEB_URL),
                "cluster_name": LOAD_TEST_CLUSTER_NAME,
                "username": LOAD_TEST_USER_PREFIX.format(user_num=self.user_num),
                "region_name": LOAD_TEST_CLUSTER_REGION,
            }
        )
        headers = {"Authorization": f"Bearer {ADMIN_TOKEN}"}
        self.start_time = time.time()
        # passing catch_response=True allows us to customize whether the request succeeded
        # we do so by calling resp.failure(msg) or resp.success()
        # warning: if you forget to report failure/success the task stats might not be reported
        for _ in range(60):
            with self.client.post(
                url=url,
                headers=headers,
                data=payload,
                catch_response=True,
            ) as resp:
                if resp.status_code == 202:
                    resp.success()
                    self.task_id = resp.json()["ID"]
                    break
                elif resp.status_code == 503:
                    resp.failure("Error - got 503")
                    # try assign request again in 1 second
                    time.sleep(1.0)
                else:
                    self.raise_fatal_exception(
                        f"Unexpected status code for assign container: {resp.status_code}"
                    )

        if self.task_id is None:
            self.raise_fatal_exception("Task ID cannot be None when polling celery")

    @locust.task
    @preprocess_locust_task
    def poll_celery(self):
        if self.task_id is None:
            self.raise_fatal_exception("Task ID cannot be None when polling celery")

        url = f"{WEB_URL}/status/{self.task_id}"
        headers = {"Authorization": f"Bearer {ADMIN_TOKEN}"}

        # passing catch_response=True allows us to customize whether the request succeeded
        # we do so by calling resp.failure(optional_str) or resp.success()
        # warning: if you forget to report failure/success the task stats might not be reported
        with self.client.get(
            url=url,
            headers=headers,
            catch_response=True,
        ) as resp:
            if resp.status_code not in (200, 503):
                msg = f"Expecting 200, got {resp.status_code}. Output: {resp.content}."
                resp.failure(msg)
                self.raise_fatal_exception(msg)
            elif resp.status_code == 503:
                # overloaded server
                resp.failure("Error - got 503")
                raise locust.exception.RescheduleTask()
            else:
                # process response
                ret = resp.json()
                state = ret["state"]
                output = ret["output"]
                if state in ("PENDING", "STARTED", "RETRY", "RECEIVED"):
                    # not a failure, just need to retry
                    resp.success()
                    raise locust.exception.RescheduleTask()
                elif state == "SUCCESS":
                    resp.success()
                    raise locust.exception.StopUser()
                else:
                    self.raise_fatal_exception(
                        f"Got unexpected state {resp['state']}. Output: {output}."
                    )

    def raise_fatal_exception(self, *args, **kwargs):
        """
        Locust tasks automatically retry on failure. In order for ValueError to properly show in the
        load test summary, we need to set a variable.
        """
        self.error = True
        raise ValueError(*args, **kwargs)

    def on_stop(self):
        self.end_time = time.time()
        save_custom_stats(user_num=self.user_num, container_time=self.end_time - self.start_time)

        # Locust (for some reason) does not neatly support stopping the test after
        # all users finish. See https://github.com/locustio/locust/issues/567.
        # We do this as a workaround
        self.add_task_finished()
        if self.get_num_finished() == TOTAL_USERS:
            self.environment.runner.quit()

    def add_task_finished(self):
        # no need for lock, see documentation for num_finished
        LoadTestUser.num_finished += 1

    def get_num_finished(self):
        return LoadTestUser.num_finished
