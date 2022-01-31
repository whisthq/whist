"""Tests for miscellaneous helper functions."""

import platform
import os
import signal
from typing import Callable

import pytest
from flask import current_app
from app.config import _callback_webserver_hostname
from app.utils.flask.flask_handlers import can_process_requests, set_web_requests_status
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
