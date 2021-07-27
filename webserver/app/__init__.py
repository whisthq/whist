import json
import logging
from time import time

from typing import Any, Callable, TypeVar, cast
from functools import wraps

from flask import current_app, request

from app.helpers.utils.general.logs import fractal_logger

from .config import _callback_webserver_hostname
from .factory import jwtManager

# Taken from https://mypy.readthedocs.io/en/stable/generics.html#declaring-decorators
_F = TypeVar("_F", bound=Callable[..., Any])


def parse_request(view_func: _F) -> _F:
    """
    Parse the incoming request and provide view_func with the following kwargs:
    - body (if POST request, the JSON parsed data)
    - recieved_from
    - webserver_url
    """

    @wraps(view_func)
    def wrapper(*args, **kwargs):
        start_time = time() * 1000
        received_from = (
            request.headers.getlist("X-Forwarded-For")[0]
            if request.headers.getlist("X-Forwarded-For")
            else request.remote_addr
        )

        # If a post body is malformed, we should treat it as an empty dict
        # that way trying to pop from it raises a KeyError, which we have proper error handling for
        try:
            # parse body if POST request and data is not an empty byte string
            if request.method == "POST" and request.data != b"":
                body = json.loads(request.data)
            else:
                body = dict()
        except:
            fractal_logger.error("Failed to parse request", exc_info=True)
            body = dict()

        kwargs["body"] = body
        kwargs["received_from"] = received_from
        kwargs["webserver_url"] = _callback_webserver_hostname()

        if request.method == "GET":
            kwargs["username"] = request.args.get("username")

        fractal_logger.debug(f"it took {time()*1000 - start_time} ms to parse this request")

        return view_func(*args, **kwargs)

    return cast(_F, wrapper)


def log_request(view_func: _F) -> _F:
    """
    Log information about the request, such as the method, URL, and body. This will no-op
    if the logging level is set below INFO.
    """

    @wraps(view_func)
    def wrapper(*args, **kwargs):
        # Don't let a logging failure kill the request processing
        start_time = time() * 1000
        try:
            # Check the log level before performing expensive computation (like stringifying the
            # body) to avoid wasted computation if it won't be logged.
            if fractal_logger.isEnabledFor(logging.INFO):
                silence = False

                for endpoint in current_app.config["SILENCED_ENDPOINTS"]:
                    if endpoint in request.url:
                        silence = True
                        break

                if not silence:
                    safe_body = ""
                    body = kwargs.get("body")
                    if body and request.method == "POST":
                        trunc_body = {k: str(v)[:500] for k, v in dict(body).items()}
                        safe_body = str(
                            {
                                k: v
                                for k, v in trunc_body.items()
                                if "password" not in k and "key" not in k
                            }
                        )

                    fractal_logger.info(
                        "{}. Body: {}".format(
                            request.method + " request at " + request.url,
                            safe_body,
                        )
                    )
        except:
            fractal_logger.error("Failed to log request", exc_info=True)
        fractal_logger.debug(f"It took {time()*1000 - start_time} ms to log this request.")
        return view_func(*args, **kwargs)

    return cast(_F, wrapper)


def fractal_pre_process(view_func: _F) -> _F:
    """
    Decorator bundling other decorators that should be set on all endpoints.

    See:
    - parse_request
    - log_request
    """

    @parse_request
    @log_request
    @wraps(view_func)
    def wrapper(*args, **kwargs):
        return view_func(*args, **kwargs)

    return cast(_F, wrapper)
