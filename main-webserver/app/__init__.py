import json
import logging
import os

from functools import wraps

import ssl
from celery import Celery
from flask import current_app, request
from flask_sendgrid import SendGrid

from app.helpers.utils.general.redis import get_redis_url

from .config import _callback_webserver_hostname
from .factory import create_app, jwtManager, ma, mail


def make_celery(app_name=__name__):
    """
    Returns a Celery object with initialized redis parameters.

    Args:
        app_name (str): the name of the app to pass to the initial Celery object

    Returns:
        an initialized `celery.Celery` instance with Redis broker/backend
    """
    redis_url = get_redis_url()
    celery_app = None
    if redis_url[:6] == "rediss":
        # use SSL
        celery_app = Celery(
            app_name,
            broker=redis_url,
            backend=redis_url,
            broker_use_ssl={
                "ssl_cert_reqs": ssl.CERT_NONE,
            },
            redis_backend_use_ssl={
                "ssl_cert_reqs": ssl.CERT_NONE,
            },
        )

    elif redis_url[:5] == "redis":
        # use regular
        celery_app = Celery(
            app_name,
            broker=redis_url,
            backend=redis_url,
        )

    else:
        # unexpected input, fail out
        raise ValueError(f"Unexpected prefix in redis url: {redis_url}")

    return celery_app


def fractal_pre_process(func):
    """
    Fractal's general endpoint preprocessing decorator. It parses the incoming request
    and provides func with the following kwargs:
        - body (if POST request, the JSON parsed data)
        - recieved_from
        - webserver_url

    Args:
        func: callback, endpoint function to be decorated
    """

    @wraps(func)
    def wrapper(*args, **kwargs):
        # We don't use X-Forwarded-For because that breaks AWS proxies
        received_from = request.remote_addr

        # If a post body is malformed, we should treat it as an empty dict
        # that way trying to pop from it raises a KeyError, which we have proper error handling for

        try:
            body = json.loads(request.data) if request.method == "POST" else dict()
        except Exception as e:
            print(str(e))
            body = dict()

        kwargs["body"] = body
        kwargs["received_from"] = received_from
        kwargs["webserver_url"] = _callback_webserver_hostname()

        silence = False

        for endpoint in current_app.config["SILENCED_ENDPOINTS"]:
            if endpoint in request.url:
                silence = True
                break

        if not silence:
            format = "%(asctime)s %(levelname)-8s [%(filename)s:%(lineno)d] %(message)s"

            logging.basicConfig(format=format, datefmt="%b %d %H:%M:%S")
            logger = logging.getLogger(__name__)
            logger.setLevel(logging.DEBUG)
            safe_body = ""

            if body and request.method == "POST":
                body = {k: str(v)[0 : min(len(str(v)), 500)] for k, v in dict(body).items()}
                safe_body = str(
                    {k: v for k, v in body.items() if "password" not in k and "key" not in k}
                )

            logger.info(
                "{}\n{}\r\n".format(
                    request.method + " " + request.url,
                    safe_body,
                )
            )

        return func(*args, **kwargs)

    return wrapper


celery_instance = make_celery()

celery_instance.set_default()

app = create_app(celery=celery_instance)
