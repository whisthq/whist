"""Tests for miscellaneous helper functions."""

from flask import current_app

from app.config import _callback_webserver_hostname


def test_callback_webserver_hostname_localhost():
    """Make sure the callback webserver hostname is that of the Heroku dev server."""

    with current_app.test_request_context():
        assert _callback_webserver_hostname() == "fractal-dev-server.herokuapp.com"


def test_callback_websesrver_hostname_not_localhost():
    """Make sure the callback webserver hostname is the same as the Host request header."""

    hostname = "fractal-prod-server.herokuapp.com"

    with current_app.test_request_context(headers={"Host": hostname}):
        assert _callback_webserver_hostname() == hostname


def test_callback_webserver_hostname_localhost_with_port():
    """Make sure the callback webserver hostname is that of the Heroku dev server."""

    with current_app.test_request_context(headers={"Host": "localhost:80"}):
        assert _callback_webserver_hostname() == "fractal-dev-server.herokuapp.com"
