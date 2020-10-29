import json
import logging
import os

from functools import wraps

from celery import Celery
from flask import request
from flask_sendgrid import SendGrid

from .factory import create_app, jwtManager, ma, mail


def make_celery(app_name=__name__):
    redis = os.environ.get("REDIS_URL", "redis://")
    return Celery(app_name, broker=redis, backend=redis)


def fractalPreProcess(f):
    @wraps(f)
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

        silence = False

        from .constants.config import SILENCED_ENDPOINTS

        for endpoint in SILENCED_ENDPOINTS:
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

        return f(*args, **kwargs)

    return wrapper


celery_instance = make_celery()
celery_instance.set_default()
app = create_app(celery=celery_instance)
