import logging


class _ExtraHandler(logging.StreamHandler):
    """
    Handles the "extra" parameter that can be passed to logging invocations. Formats any log to
    be printed as: "{function} | {label} | {message}"
    """

    def __init__(self):
        super().__init__()
        self.message_format = "{function} | {label} | {message}"

    def emit(self, record: logging.LogRecord):
        """
        This is called prior to any logging. We check which variables are
        given by the caller and fill them with defaults if not provided.
        Specifically, this handles the following parameters:
            - "function" (default: record.funcName, which is the function where logger is invoked)
            - "label" (default: None)

        Args:
            record: provided by logging library, contains info for a specific logging invocation
        """
        func = record.__dict__["function"] if "function" in record.__dict__ else record.funcName
        label = record.__dict__["label"] if "label" in record.__dict__ else None
        full_msg = self.message_format.format(function=func, label=label, message=record.msg)
        record.msg = full_msg


def _create_fractal_logger():
    """
    Create and configure a logger for fractal's purposes.
    """
    format = "%(asctime)s %(levelname)s [%(filename)s:%(funcName)s#L%(lineno)d]: %(message)s"
    logging.basicConfig(format=format, datefmt="%b %d %H:%M:%S")
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)

    # add extra handler
    extra_handler = _ExtraHandler()
    logger.addHandler(extra_handler)
    return logger


# pylint: disable=pointless-string-statement
"""
Usage:
from app.helpers.utils.general.logs import fractal_logger

fractal_logger.<loglevel>(msg)

fractal_logger can handle an extra argument that is a dictionary
containing the following supported keys:
- label

Examples:
fractal_logger.info("hi")
fractal_logger.error("oh no")
fractal_logger.error("oh no", extra={"label": "you done goofed"})
"""
fractal_logger = _create_fractal_logger()
