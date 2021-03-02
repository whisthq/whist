"""Tests for miscellaneous helper functions."""

import re

import pytest

from flask import current_app
from flask_jwt_extended import create_access_token, verify_jwt_in_request

from app.config import _callback_webserver_hostname
from app.helpers.utils.general.auth import check_developer


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
