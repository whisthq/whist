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

from scripts.utils import make_post_request, make_get_request, make_put_request
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
# a hard-coded encrypted app config token with text: fake_text, password: fake_pwd
APP_CONFIG_TOKEN = "62aed4c219c461e18f109ac689fd42e8929d82898280f9f403e78b98dacee1e5a8acad5da822a04fccc4cb890a463a2b0856e9804fa367d321f4c0e670c2057ab191383ec16ddea1d2"


class LoadTestUser(locust.HttpUser):
    # because each locust invocation is single-threaded and switches between tasks with gevent
    # so we can use variables that otherwise need lock-access:
    # number representing the largest unclaimed user number. on_start(...)
    available_user_num = 0
    # number of users that have finished.
    num_finished = 0
    # a user had an exception
    user_fail = False

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        # no need for lock, see comments on LoadTestUser.available_user_num
        self.user_num = LoadTestUser.available_user_num
        LoadTestUser.available_user_num += 1

    def on_start(self):
        # exceptions do not automatically run on_stop, so we do this as a workaround
        try:
            self._on_start()
        except Exception as e:
            print(f"User {self.user_num} got error {str(e)}")
            LoadTestUser.user_fail = True
            self.on_stop()

    def _on_start(self):
        """
        Does two things:

        1. Make request for a container
        2. Poll /host_service
        3. Send app configs to host service
        4. Poll for assign success
        """
        # make assign request
        task_id = None
        for _ in range(1):
            resp = self.try_assign_request()
            if resp.status_code == 202:
                task_id = resp.json()["ID"]
                break
            elif resp.status_code == 503:
                pass
            else:
                raise ValueError(
                    f"Unexpected status code for assign container: {resp.status_code}. Content: {resp.content}"
                )
            # sleep and try again
            time.sleep(1.0)

        assert task_id is not None

        # poll host_service
        ip = None
        host_port = None
        client_app_auth_secret = None
        for _ in range(300):
            resp = self.try_get_host_service_info()
            if resp.status_code == 200:
                resp_json = resp.json()
                ip = resp_json["ip"]
                host_port = resp_json["port"]
                client_app_auth_secret = resp_json["client_app_auth_secret"]
                if ip is not None:
                    break  # stop on non-None response
            elif resp.status_code == 503:
                pass
            else:
                raise ValueError(
                    f"unexpected status code for /host_service: {resp.status_code}. Content: {resp.content}"
                )
            # sleep and try again
            time.sleep(1.0)

        # send app config token; errors out on failure
        resp = self.send_app_config_token(ip, host_port, client_app_auth_secret)
        assert resp.ok, f"Got status code {resp.status_code} with content {resp.content}"

        # wait for container to be available; this will error out if anything fails
        poll_celery_task(WEB_URL, task_id, ADMIN_TOKEN, num_tries=60, sleep_time=1.0)

    def try_assign_request(self):
        """
        Makes a request to webserver /container/assign
        """
        payload = {
            "username": LOAD_TEST_USER_PREFIX.format(user_num=self.user_num),
            "app": "Google Chrome",
            "region": LOAD_TEST_CLUSTER_REGION,
        }
        return make_post_request(WEB_URL, "/container/assign", payload, ADMIN_TOKEN)

    def try_get_host_service_info(self):
        """
        Make a GET request to webserver /host_service for container info
        """
        params = {
            "username": LOAD_TEST_USER_PREFIX.format(user_num=self.user_num),
        }
        return make_get_request(WEB_URL, "/host_service", params=params, admin_token=ADMIN_TOKEN)

    def send_app_config_token(self, ip: str, host_port: int, client_app_auth_secret: str):
        """
        Make a PUT request to host service with app config token
        """
        host_service_url = f"https://{ip}:4678"
        payload = {
            "user_id": LOAD_TEST_USER_PREFIX.format(user_num=self.user_num),
            "host_port": host_port,
            "config_encryption_token": APP_CONFIG_TOKEN,
            "client_app_auth_secret": client_app_auth_secret,
        }
        return make_put_request(
            host_service_url, "set_config_encryption_token", payload=payload, verify=False
        )

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
            # nonzero exit code if a user failed
            self.environment.process_exit_code = 1 if LoadTestUser.user_fail is True else 0
            self.environment.runner.quit()

    def add_task_finished(self):
        # no need for lock, see documentation for LoadTestUser.num_finished
        LoadTestUser.num_finished += 1

    def get_num_finished(self):
        return LoadTestUser.num_finished
