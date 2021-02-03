"""Tests for miscellaneous helper functions."""
import time

from flask import current_app
import pytest

from app.config import _callback_webserver_hostname


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


def test_timeout_decorator_no_timeout():
    @timeout(seconds=1)
    def fast_func():
        time.sleep(0.5)

    # this should run with no timeout error
    fast_func()

    assert True


def test_timeout_decorator_yes_timeout():
    @timeout(seconds=1)
    def slow_func():
        time.sleep(1.5)

    with pytest.raises(TimeoutError):
        slow_func()
