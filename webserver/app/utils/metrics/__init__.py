"""
NOTE: this file should have NO dependencies on the Flask application. See .flask_app
for a wrapper around this that automatically handles the Flask app context.
"""
import json
import logging
from math import ceil
from typing import Union, Any, Mapping
from logzio.handler import LogzioHandler  # type: ignore


def setup_metrics_logger(logz_token: str = None, local_file: str = None) -> logging.Logger:
    """Configure a logging.Logger instance for metrics reporting.

    If logz_token is provided then metrics will be shipped to Logz using the
    LogzioHandler. This token should NOT be the Logz Custom Metrics token, it should
    be the standard log token.

    If local_file is provided then metrics will be written as newline delimited
    JSON objects to the file. This is intended for development assistance.
    """
    # we do not provide a configuration to this logger, so it will never actually print
    # anything. This also avoids overriding fracta_logger's config. However, all
    # the handlers are run so logzio/the file handler still handle the message.
    logger = logging.Logger("metrics")
    formatter = logging.Formatter(validate=False)
    if logz_token:
        logzio_handler = LogzioHandler(logz_token)
        logzio_handler.setFormatter(formatter)
        logger.addHandler(logzio_handler)
    if local_file:
        file_handler = logging.FileHandler(local_file, encoding="utf8")
        file_handler.setFormatter(formatter)
        logger.addHandler(file_handler)
    logger.setLevel(logging.DEBUG)
    return logger


def record_metrics(
    metrics_logger: logging.Logger,
    metrics: Mapping[str, Union[str, int, float, None]],
    dimensions: Mapping[str, Union[str, int, float, None]] = None,
) -> None:
    """Record a set of metrics with corresponding dimensions.

    See record_metrics in .flask_app for a variant of this method that automatically
    populates dimensions from the flask current_app.
    """
    if dimensions is None:
        dimensions = {}
    record_obj = {"metrics": metrics, "dimensions": dimensions}
    record_json = json.dumps(
        record_obj,
        separators=(",", ":"),  # recommended by json.dumps docs to get most compact serialization
    )
    metrics_logger.info(record_json)


def monotonic_duration_ms(start: float, end: float) -> int:
    """Calculate the difference between the results from two time.monotonic() calls as
    an integer value representing milliseconds elapsed.

    This integer value will be higher than the real value because it'll be rounded up.
    """
    return ceil((end - start) * 1000)
