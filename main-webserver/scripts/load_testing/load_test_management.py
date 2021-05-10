"""
Setup a load test. Supports the following methods:
- create_cluster with the name LOAD_TEST_CLUSTER_NAME
- delete_cluster with the name LOAD_TEST_CLUSTER_NAME
- make_load_test_user
- prod_webserver_context
- upgrade_webserver (used by prod_webserver_context)
- downgrade_webserver (used by prod_webserver_context)
"""
import sys
import os
import subprocess
from contextlib import contextmanager

# this adds the webserver repo root to the python path no matter where
# this file is called from. We can now import from `scripts`.
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "../.."))

from scripts.load_testing.load_test_utils import LOAD_TEST_USER_TEMPLATE

from scripts.utils import make_post_request


@contextmanager
def prod_webserver_context(app_name: str):
    """
    Creates a context that can be used to make the webserver under the Heroku app `app_name`
    production-like. See upgrade_webserver for details. Usage:
    >>> app_name = "fractal-staging-server" # example
    >>> with prod_webserver_context(app_name):
    >>>     print("The staging webserver is now prod-like")
    >>> # context is done. staging server is restored to the normal state, even if
    >>> # code in the with- block errors out

    Args:
        app_name: Name of app of webserver to scale.

    Returns:
        None. Errors out on any failure.
    """
    # before doing anything, upgrade webserver to be prod-like
    upgrade_webserver(app_name)
    try:
        # yield to the code to be run in the with- block
        yield
    finally:
        # even if there is an exception, undo the changes before raising the exception
        downgrade_webserver(app_name)


def upgrade_webserver(app_name: str):
    """
    Upgrade webserver to be prod-like. Specifically, it should run a Performance-M web dyno with
    autoscaling enabled and a 500ms desired response time.

    Args:
        app_name: Name of app of webserver to scale.

    Returns:
        None. Errors out on any failure.
    """
    ret = subprocess.run(f"heroku ps:scale web=1:Performance-M --app {app_name}", shell=True)
    assert ret.returncode == 0
    ret = subprocess.run(
        f"heroku ps:autoscale:enable --min 1 --max 10 --p95 500 --app {app_name}", shell=True
    )
    assert ret.returncode == 0


def downgrade_webserver(app_name):
    """
    Undo changes in upgrade_webserver.

    Args:
        app_name: Name of app of webserver to scale.

    Returns:
        None. Errors out on any failure.
    """
    ret = subprocess.run(f"heroku ps:scale web=1:Standard-1X --app {app_name}", shell=True)
    assert ret.returncode == 0
    ret = subprocess.run(f"heroku ps:autoscale:disable --app {app_name}", shell=True)
    assert ret.returncode == 0


def make_load_test_user(web_url: str, admin_token: str, user_num: int):
    """
    Create a user with the name LOAD_TEST_USER_TEMPLATE.format(user_num=user_num) for load testing

    Args:
        web_url: URL of the webserver to load test
        admin_token: needed to delete the cluster
        username: name of the user to create

    Returns:
        None. Errors out if anything goes wrong.
    """
    username = LOAD_TEST_USER_TEMPLATE.format(user_num=user_num)
    endpoint = "/account/register"
    payload = {
        "username": username,
        "password": username,
        "name": username,
        "feedback": username,
        "encrypted_config_token": "",
    }
    resp = make_post_request(web_url, endpoint, payload=payload, admin_token=admin_token)
    assert resp.status_code == 200, f"Got bad response code {resp.status_code}, msg: {resp.content}"
