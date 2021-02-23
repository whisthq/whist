import logging


class _MessageHandler(logging.StreamHandler):
    """
    Makes sure logged messages conform to fractal's preferred punctuation.
    """

    def __init__(self):
        super().__init__()

    def emit(self, record: logging.LogRecord):
        """
        This is called prior to any logging. We do the following checks:
        - make sure the log ends in a ".", "!", or "?"

        """
        log = record.msg
        if not log.endswith(".") and not log.endswith("!") and not log.endswith("?"):
            log = f"{log}."  # add period
            record.msg = log


class _ExtraHandler(logging.StreamHandler):
    """
    Handles the "extra" parameter that can be passed to logging invocations.
    """

    def __init__(self):
        super().__init__()

    def emit(self, record: logging.LogRecord):
        """
        This is called prior to any logging. We check which variables are
        given by the caller and fill them with defaults if not provided.
        Specifically, this handles the following parameters:
        - "function" (default: record.funcName, which is the function where logger is invoked)
        - "label" (default: None)
        """
        if "function" not in record.__dict__:
            record.__dict__["function"] = record.funcName
        if "label" not in record.__dict__:
            record.__dict__["label"] = None


def _create_fractal_logger():
    """
    Create and configure a logger for fractal's purposes.
    """
    # TODO: remove function and use funcName when fractal_log deprecated
    format = (
        "%(asctime)s %(levelname)s [%(filename)s:%(function)s#L%(lineno)d]: "
        "| %(label)s | %(message)s"
    )
    logging.basicConfig(format=format, datefmt="%b %d %H:%M:%S")
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)

    # add message handler
    message_handler = _MessageHandler()
    logger.addHandler(message_handler)

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
fractal_logger.error("oh no", extra={"label": "fake_label"})
"""
fractal_logger = _create_fractal_logger()


def fractal_log(function, label, logs, level=logging.INFO):
    """
    Deprecated old logging function.
    """
    fractal_logger.warning(f"fractal_log is deprecated. Called by function {function}.")
    fractal_logger.log(
        logs,
        level=level,
        extra={"label": label, "function": function},
    )
