"""
A singleton exposing app_record_metrics which is a wrapper around record_metrics that
auto-instantiates a metrics logger and auto-extracts default dimensions (such as
application deployment stage and version) from the Flask current_app to be sent alongside
any metrics.
"""
import logging
from typing import Mapping, Union

from flask import current_app, Flask
import app.helpers.utils.metrics.keys_dims as dkey
from . import setup_metrics_logger, record_metrics as record_metrics


def _create_metrics_logger(flask_app: Flask) -> logging.Logger:
    return setup_metrics_logger(
        logz_token=flask_app.config["METRICS_SINK_LOGZ_TOKEN"],
        local_file=flask_app.config["METRICS_SINK_LOCAL_FILE"],
    )


def _extract_default_dimensions(flask_app: Flask) -> Mapping[str, str]:
    return {
        dkey.ENVIRONMENT: str(flask_app.config["ENVIRONMENT"]),
        dkey.HOST_SERVER: str(flask_app.config["HOST_SERVER"]),
        dkey.APP_GIT_COMMIT: str(flask_app.config["APP_GIT_COMMIT"]),
        dkey.APP_NAME: "webserver",
    }


# Private singleton values instantiated and reused by app_record_metrics
_METRICS_LOGGER = None
_DEFAULT_DIMENSIONS = None


def app_record_metrics(
    metrics: Mapping[str, Union[str, int, float, None]],
    extra_dims: Mapping[str, Union[str, int, float, None]] = None,
) -> None:
    """Record a set of metrics. Dimensions extracted from the application's config will
    automatically be set, in addition to any extra_dims supplied by the callee.

    NOTE: this MUST be called only inside a context where current_app is available.
    The first time this is called it will use current_app to setup the metrics logger
    and extract default dimension values. Subsequent calls will use the cached logger
    and default dimensions.

    Example::

       app_record_metrics(
           metrics={
               "web.time_to_find_instance": time_to_find_instance,
               "web.time_to_create_row": time_to_create_row,
               "web.total_request_time": total_request_time,
           },
           extra_dims={"task_name": "assign_mandelbox"}
       )

    """
    if extra_dims is None:
        extra_dims = {}
    global _METRICS_LOGGER, _DEFAULT_DIMENSIONS  # pylint: disable=global-statement
    if _METRICS_LOGGER is None:
        _METRICS_LOGGER = _create_metrics_logger(current_app)
    if _DEFAULT_DIMENSIONS is None:
        _DEFAULT_DIMENSIONS = _extract_default_dimensions(current_app)
    record_metrics(_METRICS_LOGGER, metrics, {**_DEFAULT_DIMENSIONS, **extra_dims})
