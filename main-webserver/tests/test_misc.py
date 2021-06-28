"""Tests for miscellaneous helper functions."""

import time
import concurrent.futures
import platform
import os
import signal

import pytest
from sqlalchemy.exc import OperationalError

from flask import current_app, g
from app.config import _callback_webserver_hostname
from app.models import RegionToAmi
from app.flask_handlers import can_process_requests, set_web_requests_status
from app.helpers.utils.general.logs import fractal_logger
from app.helpers.utils.db.db_utils import set_local_lock_timeout
from app.constants.http_codes import SUCCESS, WEBSERVER_MAINTENANCE
from tests.constants import CLIENT_COMMIT_HASH_FOR_TESTING


def test_callback_webserver_hostname_localhost():
    """Make sure the callback webserver hostname is that of the Heroku dev server."""

    with current_app.test_request_context():
        assert _callback_webserver_hostname() == "dev-server.fractal.co"


def test_callback_webserver_hostname_not_localhost():
    """Make sure the callback webserver hostname is the same as the Host request header."""

    hostname = "google.com"

    with current_app.test_request_context(headers={"Host": hostname}):
        assert _callback_webserver_hostname() == hostname


def test_callback_webserver_hostname_localhost_with_port():
    """Make sure the callback webserver hostname is that of the Heroku dev server."""

    with current_app.test_request_context(headers={"Host": "localhost:80"}):
        assert _callback_webserver_hostname() == "dev-server.fractal.co"


# this test cannot be run on windows, as it uses POSIX signals.
@pytest.mark.skipif(
    "windows" in platform.platform().lower(), reason="must be running a POSIX compliant OS."
)
def test_webserver_sigterm(client):
    """
    Make sure SIGTERM is properly handled by webserver. After a SIGTERM, all new web requests should
    error out with code WEBSERVER_MAINTENANCE. For more info, see app/signals.py.

    Note that this is not the perfect test; third party libs like waitress can override signal
    handlers. Waitress is only used in deployments (local or in Procfile with Heroku), not during
    tests. It has been independently verified that waitress does not override our SIGTERM handler.
    """
    # this is a dummy endpoint that we hit to make sure web requests are ok
    resp = client.post("/newsletter/post")
    assert resp.status_code == SUCCESS

    self_pid = os.getpid()
    os.kill(self_pid, signal.SIGTERM)

    assert not can_process_requests()

    # web requests should be rejected
    resp = client.post("/newsletter/post")
    assert resp.status_code == WEBSERVER_MAINTENANCE

    # re-enable web requests
    assert set_web_requests_status(True)

    # should be ok
    resp = client.post("/newsletter/post")
    assert resp.status_code == SUCCESS


def test_rate_limiter(client):
    """
    Test the rate limiter decorator. The first 10 requests should succeed,
    but the 11th should error out with 429.
    """
    for _ in range(10):
        resp = client.post("/newsletter/post")
        assert resp.status_code == 200
        g._rate_limiting_complete = False

    resp = client.post("/newsletter/post")
    assert resp.status_code == 429


def test_local_lock_timeout(app):
    """
    Test the function `set_local_lock_timeout` by running concurrent threads that try to grab
    the lock. One should time out.
    """

    def acquire_lock(lock_timeout: int, hold_time: int):
        try:
            with app.app_context():
                set_local_lock_timeout(lock_timeout)
                _ = RegionToAmi.query.with_for_update().get(
                    ("us-east-1", CLIENT_COMMIT_HASH_FOR_TESTING)
                )
                fractal_logger.info("Got lock and data")
                time.sleep(hold_time)
            return True
        except OperationalError as op_err:
            if "lock timeout" in str(op_err):
                return False
            else:
                # if we get an unexpected error, this test will fail
                raise op_err

    with concurrent.futures.ThreadPoolExecutor() as executor:
        fractal_logger.info("Starting threads...")
        thread_one = executor.submit(acquire_lock, 5, 10)
        thread_two = executor.submit(acquire_lock, 5, 10)

        fractal_logger.info("Getting thread results...")
        thread_one_result = thread_one.result()
        thread_two_result = thread_two.result()

        if thread_one_result is True and thread_two_result is True:
            fractal_logger.error("Both threads got the lock! Locking failed.")
            assert False
        elif thread_one_result is False and thread_two_result is False:
            fractal_logger.error("Neither thread got the lock! Invesigate..")
            assert False
        # here, only one thread got the lock so this test succeeds


@pytest.mark.usefixtures("authorized")
def test_regions(client):
    """Ensure that regions are returned by the /regions endpoint if they are allwed regions."""

    response = client.get("/regions")
    region_set = ["us-east-1", "us-west-1", "us-west-2", "ca-central-1", "us-east-2"]

    assert any(item in region_set for item in response.json)
