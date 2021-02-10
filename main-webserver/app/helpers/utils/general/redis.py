import os
import logging
import ssl

import redis
from func_timeout import func_set_timeout, FunctionTimedOut

from app.helpers.utils.general.logs import fractal_log


def get_redis_url():
    """
    Gets the Redis URL from the environment. This can either be REDIS_TLS_URL
    or REDIS_URL. Heroku can be inconsistent in which it gives us, even if
    both are pointing to a Redis+TLS instance.

    Returns:
        (str) the redis URL to connect to
    """
    redis_tls_url = os.environ.get("REDIS_TLS_URL", "")
    redis_url = os.environ.get("REDIS_URL", "")
    if redis_tls_url == "":
        fractal_log(
            "get_redis_url",
            "",
            "Could not find REDIS_TLS_URL. Will try rediss://",
            level=logging.WARNING,
        )
        # default
        redis_tls_url = "rediss://"

    if redis_url == "":
        fractal_log(
            "get_redis_url",
            "",
            "Could not find REDIS_URL. Will try redis://",
            level=logging.WARNING,
        )
        # default
        redis_url = "redis://"

    if try_redis_url(redis_tls_url):
        fractal_log(
            "get_redis_url",
            None,
            f"REDIS_URL: {redis_tls_url}",
        )
        return redis_tls_url
    if try_redis_url(redis_url):
        fractal_log(
            "get_redis_url",
            None,
            f"REDIS_URL: {redis_url}",
        )
        return redis_url
    else:
        raise ValueError("No valid redis URL could be found.")


def try_redis_url(redis_url):
    """
    Tries a redis_url. Can be SSL supported (redis://) or regular (redis://).
    Args:
        redis_url (str): url to try
    Returns:
        (bool) True if valid url, False otherwise
    """

    redis_conn = None

    if redis_url[:6] == "rediss":
        # use SSL
        redis_conn = redis.from_url(redis_url, ssl_cert_reqs=ssl.CERT_NONE)

    elif redis_url[:5] == "redis":
        # use regulaar Redis
        redis_conn = redis.from_url(redis_url)

    else:
        # unrecognized prefix
        raise ValueError(f"Unexpected redis url: {redis_url}")

    try:
        _test_redis_url(redis_conn)
        return True
    except FunctionTimedOut:
        # this can happen with SSL. Just return False.
        return False
    except redis.exceptions.ConnectionError:
        # this can happen with regular redis. Just return False
        return False
    # any other code flow will be an unexpected error and will be passed to the caller


@func_set_timeout(timeout=1)
def _test_redis_url(redis_conn):
    redis_conn.ping()
    redis_conn.close()
