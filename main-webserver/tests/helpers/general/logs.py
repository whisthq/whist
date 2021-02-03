import sys
import logging
import os


def fractal_log(label, logs, level=logging.INFO):
    # efficiently retrieve caller name and line number
    frame = sys._getframe().f_back # pylint: disable=protected-access
    caller_filename = frame.f_code.co_filename
    caller_funcname = frame.f_code.co_name
    caller_lineno = frame.f_lineno
    # format = "%(asctime)s %(levelname)-8s [%(filename)s:%(lineno)d] %(message)s"
    format = "%(asctime)s %(levelname)-8s %(message)s"

    logging.basicConfig(format=format, datefmt="%b %d %H:%M:%S")
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)

    if not logs.endswith(".") and not logs.endswith("!") and not logs.endswith("?"):
        logs = "{logs}.".format(logs=logs)

    output = "{filename}:{function}#L{lineno} | {label} | {logs}".format(
        filename=caller_filename,
        function=str(caller_funcname),
        lineno=caller_lineno,
        label=str(label),
        logs=str(logs),
    )

    if level == logging.CRITICAL:
        logger.critical(output)
    elif level == logging.ERROR:
        logger.error(output)
    elif level == logging.WARNING:
        logger.warning(output)
    elif level == logging.INFO:
        logger.info(output)
    else:
        logger.debug(output)


def newLine():
    print("\n")
