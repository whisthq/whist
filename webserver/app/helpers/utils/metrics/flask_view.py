from typing import Mapping, Optional
import time

import flask

from app.helpers.utils.general.logs import fractal_logger
from app.helpers.utils.metrics import monotonic_duration_ms
from app.helpers.utils.metrics.flask_app import app_record_metrics
import app.helpers.utils.metrics.keys as mkey
import app.helpers.utils.metrics.keys_dims as dkey


# This should be a globally unique key that shouldn't conflict with any other
# potential keys in flask.request.
ATTR__REQUEST_START = "Fractal-Request-StartTime"


def _get_dims_from_request(request: flask.Request) -> Mapping[str, Optional[str]]:
    """Extract relevant dimensions to send along with metrics for a given request, such as
    the requested endpoint.
    """
    return {dkey.WEB_REQ_ENDPOINT: request.endpoint}


def _before_request_metrics():
    """Instrument a request with various attributes for later usage to produce request metrics"""
    # time.monotonic is only valid when compared against other time.monotonic calls
    setattr(flask.request, ATTR__REQUEST_START, time.monotonic())


def _after_request_metrics(response: flask.Response):
    """Extract information from a request that's been handled and record various metrics
    such as the request duration and response code.
    """
    # Don't let an error when recording metrics impact returning the response to the user
    try:
        metrics = {}
        if start_time := getattr(flask.request, ATTR__REQUEST_START, None):
            metrics[mkey.WEB__REQUEST_DURATION__MS] = monotonic_duration_ms(
                start_time, time.monotonic()
            )
        app_record_metrics(
            metrics,
            extra_dims={
                **_get_dims_from_request(flask.request),
                dkey.WEB_REQ_STATUS_CODE: response.status_code,
            },
        )
    except Exception as e:
        fractal_logger.error(e)

    return response


def register_flask_view_metrics_monitor(flask_app: flask.Flask) -> None:
    """Register hooks around Flask's request handling to automatically collect metrics
    about requests, such as the time it took to process the request.
    """
    flask_app.before_request(_before_request_metrics)
    flask_app.after_request(_after_request_metrics)
