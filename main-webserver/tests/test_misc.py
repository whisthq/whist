"""Tests for miscellaneous helper functions."""

import re
import time
import concurrent.futures
import platform
import os
import signal

import pytest
from sqlalchemy.exc import OperationalError

from flask import current_app, g
from flask_jwt_extended import create_access_token, verify_jwt_in_request

from celery import current_app as current_celery_app

from app.config import _callback_webserver_hostname
from app import can_process_requests, set_web_requests_status
from app.helpers.utils.general.auth import check_developer
from app.helpers.utils.general.logs import fractal_logger
from app.helpers.utils.aws.utils import Retry, retry_with_backoff
from app.helpers.utils.db.db_utils import set_local_lock_timeout
from app.models import db, RegionToAmi
from app.constants.http_codes import SUCCESS, WEBSERVER_MAINTENANCE
from app.constants.http_codes import SUCCESS, ACCEPTED, WEBSERVER_MAINTENANCE


def test_callback_webserver_hostname_localhost():
    """Make sure the callback webserver hostname is that of the Heroku dev server."""

    with current_app.test_request_context():
        assert _callback_webserver_hostname() == "dev-server.fractal.co"


def test_callback_websesrver_hostname_not_localhost():
    """Make sure the callback webserver hostname is the same as the Host request header."""

    hostname = "google.com"

    with current_app.test_request_context(headers={"Host": hostname}):
        assert _callback_webserver_hostname() == hostname


def test_callback_webserver_hostname_localhost_with_port():
    """Make sure the callback webserver hostname is that of the Heroku dev server."""

    with current_app.test_request_context(headers={"Host": "localhost:80"}):
        assert _callback_webserver_hostname() == "dev-server.fractal.co"


# this test cannot be run on windows, as it uses POSIX signals.
# note that this is not the best test; third party libs like waitress can override signal handlers
# this test runs in a flask context, so it might appear to work.
# it has been independently verified that waitress does not override our SIGTERM handler.
@pytest.mark.skipif(
    "windows" in platform.platform().lower(), reason="must be running a POSIX compliant OS."
)
def test_webserver_sigterm(client):
    # this is a dummy endpoint that we hit to make sure web requests are ok
    resp = client.post("/newsletter/post")
    assert resp.status_code == SUCCESS

    self_pid = os.getpid()
    os.kill(self_pid, signal.SIGTERM)

    # # we need to wait for SIGTERM to come
    # for _ in range(100):
    #     if not can_process_requests():
    #         break
    #     time.sleep(0.5)
    assert not can_process_requests()

    # web requests should be rejected
    resp = client.post("/newsletter/post")
    assert resp.status_code == WEBSERVER_MAINTENANCE

    # re-enable web requests
    assert set_web_requests_status(True)

    # should be ok
    resp = client.post("/newsletter/post")
    assert resp.status_code == SUCCESS


@pytest.mark.parametrize(
    "username, is_developer",
    (
        ("developer@fractal.co", True),
        ("jeff@amazon.com", False),
        ("h4x0r@fractal.co@gmail.com", False),
    ),
)
def test_check_developer(app, is_developer, username):
    """Handle authorized requests from normal users correctly."""

    access_token = create_access_token(username)

    with app.test_request_context("/", headers={"Authorization": f"Bearer {access_token}"}):
        verify_jwt_in_request()

        assert check_developer() == is_developer


def test_check_admin(app):
    """Make sure check_developer() returns True when the requester is the admin user."""

    access_token = create_access_token(app.config["DASHBOARD_USERNAME"])

    with app.test_request_context("/", headers={"Authorization": f"Bearer {access_token}"}):
        verify_jwt_in_request()

        assert check_developer() == True


def test_check_authorization_optional(app):
    """Make sure check_developer() returns False within an unauthorized request's context."""

    verify_jwt_in_request(optional=True)

    assert check_developer() == False


def test_check_before_verify():
    """Make sure check_developer() raises a RuntimeError when called first.

    Specifically, make sure check_developer() raises a RuntimeError when called before both
    @jwt_required() and verify_jwt_in_request() while processing an unauthenticated request.
    """

    with pytest.raises(
        RuntimeError,
        match=re.escape(
            "You must call `@jwt_required()` or `verify_jwt_in_request` before using this method"
        ),
    ):
        check_developer()


def test_retry():
    """Retry a function that fails the first two times it is called and succeeds the third time."""

    counter = {"count": 0}  # Keep track of the number of times the function is called.

    @retry_with_backoff(min_wait=1, wait_delta=0, max_wait=1, max_retries=2)
    def retry_me(counter):
        counter["count"] += 1

        try:
            assert counter["count"] > 2  # Fail the first two times the function is called.
        except AssertionError as error:
            raise Retry from error

        return counter["count"]

    start = time.time()
    count = retry_me(counter)
    end = time.time()

    assert count == 3  # The function was called three times in total.
    assert int(end - start) == 2  # There was a one second wait in between each call attempt.


def test_retry_timeout():
    """Retry a function that never returns successfully.

    This test should take five seconds to complete because the @retry_with_backoff() decorator will
    wait for one second after its first attempt to call the function, two seconds after the second
    attempt, and two seconds after its third attempt.
    """

    counter = {"count": 0}  # Keep track of the number of times the function is called.

    @retry_with_backoff(min_wait=1, wait_delta=1, max_wait=2, max_retries=3)
    def timeout(counter):
        counter["count"] += 1

        try:
            raise Exception("Hello, world!")
        except Exception as exc:
            raise Retry from exc

    start = time.time()

    with pytest.raises(Exception, match=r"Hello, world!"):
        timeout(counter)

    end = time.time()

    assert counter["count"] == 4  # The function is called four times in total.
    assert int(end - start) == 5


def test_rate_limiter(client):
    """
    Test the rate limiter decorator. The first 10 requests should succeed,
    but the 11th should error out with 429.
    """
    for i in range(10):
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
                _ = RegionToAmi.query.with_for_update().get("us-east-1")
                fractal_logger.info("Got lock and data")
                time.sleep(hold_time)
            return True
        except OperationalError as oe:
            if "lock timeout" in str(oe):
                return False
            else:
                # if we get an unexpected error, this test will fail
                raise oe

    with concurrent.futures.ThreadPoolExecutor() as executor:
        fractal_logger.info("Starting threads...")
        t1 = executor.submit(acquire_lock, 5, 10)
        t2 = executor.submit(acquire_lock, 5, 10)

        fractal_logger.info("Getting thread results...")
        t1_result = t1.result()
        t2_result = t2.result()

        if t1_result is True and t2_result is True:
            fractal_logger.error("Both threads got the lock! Locking failed.")
            assert False
        elif t1_result is False and t2_result is False:
            fractal_logger.error("Neither thread got the lock! Invesigate..")
            assert False
        # here, only one thread got the lock so this test succeeds
