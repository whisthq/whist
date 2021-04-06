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
import gevent

from scripts.load_testing.load_test_utils import (
    LOAD_TEST_CLUSTER_REGION,
    LOAD_TEST_USER_PREFIX,
    get_task_definition_arn,
    OUTFOLDER,
    CSV_PREFIX,
)

from scripts.utils import make_post_request
from scripts.celery_scripts import poll_celery_task


def get_num_users_and_host():
    """
    -u (number of users), --host (webserver url) are passed to locust via CLI args. However, Locust
    does not provide a way to access these variables. So, we extract them ourselves.
    """
    # figured this out by looking at main() in locust source
    options = locust.argument_parser.parse_options()
    num_users = options.num_users
    host = options.host
    return num_users, host


TOTAL_USERS, WEB_URL = get_num_users_and_host()
ADMIN_TOKEN = os.environ["ADMIN_TOKEN"]


class LoadTestUser(locust.HttpUser):
    # because each locust invocation is single-threaded and switches between tasks with gevent
    # so we can use variables that otherwise need lock-access:
    # number representing the largest unclaimed user number. on_start(...)
    available_user_num = 0
    # number of users that have finished.
    num_finished = 0

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        # no need for lock, see comments on LoadTestUser.available_user_num
        self.user_num = LoadTestUser.available_user_num
        LoadTestUser.available_user_num += 1

    def on_start(self):
        # exceptions do not automatically run on_stop, so we do this as a workaround
        try:
            self._on_start()
        except:
            self.on_stop()

    def _on_start(self):
        """
        Does two things:

        1. Make request for a container
        2. Poll until it is available
        """
        task_id = None
        for _ in range(60):
            resp = self.try_assign_request()
            if resp.status_code == 202:
                task_id = resp.json()["ID"]
            elif resp.status_code == 503:
                pass
            else:
                raise ValueError(f"Unexpected status code for assign container: {resp.status_code}")
            # sleep and try again
            time.sleep(1.0)

        # wait for container to be available; this will error out if anything fails
        # TODO: support app config
        poll_celery_task(WEB_URL, task_id, ADMIN_TOKEN, num_tries=300, sleep_time=1.0)

    def try_assign_request(self):
        """
        Makes a request to /aws_container/assign_container
        """
        payload = {
            "task_definition_arn": get_task_definition_arn(WEB_URL),
            "cluster_name": None,  # None means a cluster is chosen, like in /container/assign
            "username": LOAD_TEST_USER_PREFIX.format(user_num=self.user_num),
            "region_name": LOAD_TEST_CLUSTER_REGION,
        }
        return make_post_request(WEB_URL, "/aws_container/assign_container", payload, ADMIN_TOKEN)

    @locust.task
    def do_nothing(self):
        # indicates this user is done doing work; transitions user to on_stop
        raise locust.exception.StopUser()

    def on_stop(self):
        # Locust (for some reason) does not neatly support stopping the test after
        # all users finish/error out. See https://github.com/locustio/locust/issues/567.
        # We do this as a workaround
        self.add_task_finished()
        if self.get_num_finished() == TOTAL_USERS:
            self.environment.runner.quit()

    def add_task_finished(self):
        # no need for lock, see documentation for LoadTestUser.num_finished
        LoadTestUser.num_finished += 1

    def get_num_finished(self):
        return LoadTestUser.num_finished
