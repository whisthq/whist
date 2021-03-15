from typing import Any, Callable, TypeVar, Tuple, cast
import json
import threading
from functools import wraps
import logging

from flask import (
    current_app,
    request,
    make_response,
    jsonify,
)

from app.helpers.utils.general.logs import fractal_logger
from app.constants.http_codes import WEBSERVER_MAINTENANCE

from .config import _callback_webserver_hostname
from .factory import create_app, jwtManager, ma, mail

_WEB_REQUESTS_ENABLED = True
_WEB_REQUESTS_LOCK = threading.Lock()


def set_web_requests_status(enabled: bool) -> bool:
    """
    Set state of _WEB_REQUESTS_ENABLED.
    Args:
        enabled: State of _WEB_REQUESTS_ENABLED. If True, web requests allowed. If False, all are
            rejected with WEBSERVER_MAINTENANCE status code.
    Returns:
        True iff _WEB_REQUESTS_ENABLED was set to enabled.
    """
    global _WEB_REQUESTS_ENABLED
    has_lock = _WEB_REQUESTS_LOCK.acquire(blocking=True, timeout=1)
    if not has_lock:
        # this should not happen and means our lock contention is bad
        fractal_logger.error(
            f"Could not acquire web requests lock. Could not set _WEB_REQUESTS_ENABLED={enabled}."
        )
        return False
    _WEB_REQUESTS_ENABLED = enabled
    _WEB_REQUESTS_LOCK.release()
    fractal_logger.info(f"_WEB_REQUESTS_ENABLED set to {enabled}")
    return True


def can_process_requests() -> bool:
    has_lock = _WEB_REQUESTS_LOCK.acquire(blocking=True, timeout=1)
    if not has_lock:
        # this should not happen and means our lock contention is bad
        fractal_logger.error(
            "Could not acquire web requests lock. Assuming requests cannot be handled."
        )
        return False
    # copy value into web_requests_status
    web_requests_status = _WEB_REQUESTS_ENABLED
    _WEB_REQUESTS_LOCK.release()
    return web_requests_status


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
        if not can_process_requests():
            return make_response(
                jsonify({"msg": "Webserver is not processing requests right now."}),
                WEBSERVER_MAINTENANCE,
            )
        received_from = (
            request.headers.getlist("X-Forwarded-For")[0]
            if request.headers.getlist("X-Forwarded-For")
            else request.remote_addr
        )

        # If a post body is malformed, we should treat it as an empty dict
        # that way trying to pop from it raises a KeyError, which we have proper error handling for
        try:
            body = json.loads(request.data) if request.method == "POST" else dict()
        except:
            fractal_logger.error("Failed to parse request", exc_info=True)
            body = dict()

        kwargs["body"] = body
        kwargs["received_from"] = received_from
        kwargs["webserver_url"] = _callback_webserver_hostname()

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
