import os

import sentry_sdk
from sentry_sdk.integrations.flask import FlaskIntegration
from sentry_sdk.integrations.celery import CeleryIntegration

from app.exceptions import SentryInitializationError


def init_and_ensure_sentry_connection(env: str, sentry_dsn: str):
    """
    Initialized sentry with Flask and Celery integrations as well as the default
    integrations. Also makes sure initialization is correct. We do this by logging
    a test message at the least serious level (debug). According to the docs and
    manual experimentation, the return is None iff something went wrong. Otherwise,
    the return is the ID of the message.

    Args:
        env: Acquired from HEROKU_APP_NAME env var. One of
            ["fractal-prod-server", "fractal-staging-server"]
        sentry_dsn: Acquired from SENTRY_DSN env var. Points to external sentry resource.

    Return:
        Errors out if the client is not initialized correctly.
    """
    sentry_sdk.init(
        dsn=sentry_dsn,
        integrations=[FlaskIntegration(), CeleryIntegration()],
        environment=env,
        release="main-webserver@" + os.getenv("HEROKU_SLUG_COMMIT", "local"),
    )
    # Docs:
    # https://getsentry.github.io/sentry-python/api.html?highlight=capture_message#sentry_sdk.capture_message
    resp = sentry_sdk.capture_message("WEBSERVER SENTRY INITIALIZATION TEST MESSAGE", level="debug")
    if resp is None:
        raise SentryInitializationError(
            f"Failed to initialize sentry DSN with env: {env}, dsn: {sentry_dsn}"
        )
