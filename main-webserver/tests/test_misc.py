"""Tests for miscellaneous helper functions."""
import os
import ssl

from celery import Celery
from flask import current_app

from app.config import _callback_webserver_hostname
from app.helpers.utils.general.time import timeout


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


# @timeout(seconds=5, error_message="FAILED REDIS CONN TEST, ANY REDIS TESTS WILL HANG")
# def test_redis_conn():
#     """
#     Make sure Redis connection exists. Make sure this configuration matches make_celery() and celery_config()
#     TODO: can we merge all these configurations to a single function for prod and testing?
#     """
#     redis_url = os.environ.get("REDIS_TLS_URL", "rediss://")

#     celery_app = Celery(
#         "",
#         broker=redis_url,
#         backend=redis_url,
#         broker_use_ssl={
#             "ssl_cert_reqs": ssl.CERT_NONE,
#         },
#         redis_backend_use_ssl={
#             "ssl_cert_reqs": ssl.CERT_NONE,
#         },
#     )

#     # this ping checks to see if redis is available. SSL connections just
#     # freeze if the instance is not properly set up, so this method is
#     # wrapped in a timeout.
#     celery_app.control.inspect().ping()

#     return True
