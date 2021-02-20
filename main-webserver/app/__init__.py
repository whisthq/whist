import json
import logging

from functools import wraps

from flask import current_app, request

from .config import _callback_webserver_hostname
from .factory import create_app, jwtManager, ma, mail


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
