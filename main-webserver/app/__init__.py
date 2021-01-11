import json
import logging
import os

from functools import wraps

import ssl
from celery import Celery
from flask import current_app, request
from flask_sendgrid import SendGrid

from app.helpers.utils.general.time import timeout
from app.helpers.utils.general.logs import fractal_log

from .config import _callback_webserver_hostname
from .factory import create_app, jwtManager, ma, mail


@timeout(seconds=5, error_message="could not initialize celery with redis")
def make_celery(app_name=__name__):
    redis = os.environ.get("REDIS_TLS_URL", "rediss://")

    celery_app = Celery(
        app_name,
        broker=redis,
        backend=redis,
        broker_use_ssl={
            "ssl_cert_reqs": ssl.CERT_NONE,
        },
        redis_backend_use_ssl={
            "ssl_cert_reqs": ssl.CERT_NONE,
        },
    )

    # this ping checks to see if redis is available. SSL connections just
    # freeze if the instance is not properly set up, so this method is
    # wrapped in a timeout. TODO: actually look at return from .ping()
    celery_app.control.inspect().ping()

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
        received_from = (
            request.headers.getlist("X-Forwarded-For")[0]
            if request.headers.getlist("X-Forwarded-For")
            else request.remote_addr
        )

        try:
            body = json.loads(request.data) if request.method == "POST" else None
        except Exception as e:
            print(str(e))
            body = None

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
