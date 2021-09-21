import os

import sentry_sdk
from sentry_sdk.integrations.flask import FlaskIntegration

from app.exceptions import SentryInitializationError


def init_and_ensure_sentry_connection(env: str, sentry_dsn: str):
    """
    Initialized sentry with Flask integration as well as the default
    integrations. Also makes sure initialization succeeds. We do this by logging
    a test message at the least serious level (debug). According to the docs and
    manual experimentation, the return is None if and only if something went wrong. Otherwise,
    the return is the ID of the message.

    Args:
        env: A valid value from app.constants.env_names
        sentry_dsn: Acquired from SENTRY_DSN env var. Points to external sentry resource.

    Return:
        Errors out if the client is not initialized correctly.
    """
    sentry_sdk.init(  # pylint: disable=abstract-class-instantiated
        dsn=sentry_dsn,
        integrations=[FlaskIntegration()],
        environment=env,
        release="webserver@" + os.getenv("HEROKU_SLUG_COMMIT", "local"),  # FIXME no env usage
    )

    # If you want to test Sentry, uncomment the following
    # Docs: https://getsentry.github.io/sentry-python/api.html?highlight=capture_message#sentry_sdk.capture_message
    # resp = sentry_sdk.capture_message("WEBSERVER SENTRY INITIALIZATION TEST MESSAGE", level="debug")
