import os
import logging

from app.helpers.utils.general.logs import fractal_log


def get_redis_url():
    """
    Gets the Redis URL from the environment. This can either be REDIS_TLS_URL
    or REDIS_URL. Heroku can be inconsistent in which it gives us, even if
    both are pointing to a Redis+TLS instance.
    """

    redis_url = os.environ.get("REDIS_TLS_URL", "")  # should look like rediss://<something>
    if redis_url == "":
        redis_url = os.environ.get("REDIS_URL", "")
        if redis_url == "":
            fractal_log(
                "get_redis_url",
                "",
                "Could not find REDIS_TLS_URL or REDIS_URL in env. Using rediss://",
                level=logging.WARNING,
            )
            # default
            redis_url = "rediss://"

    return redis_url
