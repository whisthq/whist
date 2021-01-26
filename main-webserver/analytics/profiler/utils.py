import os
import signal
from functools import wraps
import errno
import ssl

import redis


# from https://stackoverflow.com/questions/2281850/timeout-function-if-it-takes-too-long-to-finish
class TimeoutError(Exception):
    """
    Generic Timeout class. This is called if any function using the @timeout decorator
    times out. It inherits from Exception, so any args passed go right to the parent class.
    """


# from https://stackoverflow.com/questions/2281850/timeout-function-if-it-takes-too-long-to-finish
def timeout(seconds, error_message=os.strerror(errno.ETIMEDOUT)):
    """
    Decorator to make a timeout for long-running functions. This is necessary
    because Python's SSL library seems to not handle handshake timeouts (see:
    https://stackoverflow.com/questions/19938593/web-app-hangs-for-several-hours-in-ssl-py-at-self-sslobj-do-handshake
    )
    Unfortunately, this means we need to do the timeout ourselves.
    Args:
        TODO: higher res timers (https://docs.python.org/3/library/signal.html#signal.setitimer)
        seconds (int): seconds to before TimeoutError raised
        error_message (str): what to print after an error
    Usage:
    @timeout(seconds=NUM_SEC, error_message="Whatever you want")
    def long_running_func()
    """

    def timer_decorator(func):
        def _handle_timeout(signum, frame):
            raise TimeoutError(error_message)

        @wraps(func)
        def wrapper(*args, **kwargs):
            signal.signal(signal.SIGALRM, _handle_timeout)
            signal.alarm(seconds)
            try:
                result = func(*args, **kwargs)
            finally:
                signal.alarm(0)
            return result

        return wrapper

    return timer_decorator


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
        print("Could not find REDIS_TLS_URL. Will try rediss://")
        # default
        redis_tls_url = "rediss://"

    if redis_url == "":
        print("Could not find REDIS_URL. Will try redis://")
        # default
        redis_url = "redis://"

    if try_redis_url(redis_tls_url):
        return redis_tls_url
    if try_redis_url(redis_url):
        return redis_url
    else:
        raise ValueError("No valid redis URL could be found.")


@timeout(seconds=1)
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
        # this ping checks to see if redis is available. SSL connections just
        # freeze if the instance is not properly set up, so this method is
        # wrapped in a timeout.
        redis_conn.ping()
        redis_conn.close()
        return True
    except TimeoutError:
        # this can happen with SSL. Just return False.
        return False
    except redis.exceptions.ConnectionError:
        # this can happen with regular redis. Just return False
        return False

    # any other code flow will be an unexpected error and will be passed to the caller
