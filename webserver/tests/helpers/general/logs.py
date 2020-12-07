import logging
import os


def fractalLog(function, label, logs, level=logging.INFO):
    format = "%(asctime)s %(levelname)-8s [%(filename)s:%(lineno)d] %(message)s"

    logging.basicConfig(format=format, datefmt="%b %d %H:%M:%S")
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)

    if not logs.endswith(".") and not logs.endswith("!") and not logs.endswith("?"):
        logs = "{logs}.".format(logs=logs)

    output = "{function} | {label} | {logs}".format(
        function=str(function), label=str(label), logs=str(logs)
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
