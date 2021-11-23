"""Tests for miscellaneous helper functions."""

import time
import concurrent.futures
import platform
import os
import signal
from typing import Callable, Optional

import pytest
from sqlalchemy.exc import OperationalError

from flask import current_app, Flask
from app.config import _callback_webserver_hostname
from app.database.models.cloud import RegionToAmi
from app.utils.flask.flask_handlers import can_process_requests, set_web_requests_status
from app.utils.general.logs import whist_logger
from app.utils.db.db_utils import set_local_lock_timeout
from tests.constants import CLIENT_COMMIT_HASH_FOR_TESTING
from tests.client import WhistAPITestClient


def test_callback_webserver_hostname_localhost() -> None:
    """Make sure the callback webserver hostname is that of the Heroku dev server."""

    with current_app.test_request_context():
        assert _callback_webserver_hostname() == "dev-server.whist.com"


def test_callback_webserver_hostname_not_localhost() -> None:
    """Make sure the callback webserver hostname is the same as the Host request header."""

    hostname = "google.com"

    with current_app.test_request_context(headers={"Host": hostname}):
        assert _callback_webserver_hostname() == hostname


def test_callback_webserver_hostname_localhost_with_port() -> None:
    """Make sure the callback webserver hostname is that of the Heroku dev server."""

    with current_app.test_request_context(headers={"Host": "localhost:80"}):
        assert _callback_webserver_hostname() == "dev-server.whist.com"


# this test cannot be run on windows, as it uses POSIX signals.
@pytest.mark.skipif(
    "windows" in platform.platform().lower(), reason="must be running a POSIX compliant OS."
)
def test_webserver_sigterm(client: WhistAPITestClient, make_user: Callable[..., str]) -> None:
    """
    Make sure SIGTERM is properly handled by webserver. After a SIGTERM, all new web requests should
    error out with code RESOURCE_UNAVAILABLE. For more info, see app/signals.py.

    Note that this is not the perfect test; third party libs like waitress can override signal
    handlers. Waitress is only used in deployments (local or in Procfile with Heroku), not during
    tests. It has been independently verified that waitress does not override our SIGTERM handler.
    """

    user = make_user()

    client.login(user)

    self_pid = os.getpid()
    os.kill(self_pid, signal.SIGTERM)

    assert not can_process_requests()

    # re-enable web requests
    assert set_web_requests_status(True)


def test_local_lock_timeout(app: Flask, region_name: Optional[str]) -> None:
    """
    Test the function `set_local_lock_timeout` by running concurrent threads that try to grab
    the lock. One should time out.
    """

    def acquire_lock(lock_timeout: int, hold_time: int) -> bool:
        try:
            with app.app_context():
                set_local_lock_timeout(lock_timeout)
                _ = RegionToAmi.query.with_for_update().get(
                    (region_name, CLIENT_COMMIT_HASH_FOR_TESTING)
                )
                whist_logger.info("Got lock and data")
                time.sleep(hold_time)
            return True
        except OperationalError as op_err:
            if "lock timeout" in str(op_err):
                return False
            else:
                # if we get an unexpected error, this test will fail
                raise op_err

    with concurrent.futures.ThreadPoolExecutor() as executor:
        whist_logger.info("Starting threads...")
        thread_one = executor.submit(acquire_lock, 5, 10)
        thread_two = executor.submit(acquire_lock, 5, 10)

        whist_logger.info("Getting thread results...")
        thread_one_result = thread_one.result()
        thread_two_result = thread_two.result()

        if thread_one_result is True and thread_two_result is True:
            whist_logger.error("Both threads got the lock! Locking failed.")
            assert False
        elif thread_one_result is False and thread_two_result is False:
            whist_logger.error("Neither thread got the lock! Invesigate..")
            assert False
        # here, only one thread got the lock so this test succeeds
