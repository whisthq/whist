import logging
import timber
import os


def fractalLog(logs, level=logging.INFO):
    format = "%(asctime)s %(levelname)-8s [%(filename)s:%(lineno)d] %(message)s"
    logging.basicConfig(format=format, datefmt="%b %d %H:%M:%S")

    logger = logging.getLogger(__name__)

    timber_handler = timber.TimberHandler(
        source_id=str(os.getenv("TIMBER_SOURCE_ID")),
        api_key=str(os.getenv("TIMBER_API_KEY")),
    )
    logger.setLevel(logging.DEBUG)
    logger.addHandler(timber_handler)

    if level == logging.CRITICAL:
        logger.critical(logs)
    elif level == logging.ERROR:
        logger.error(logs)
    elif level == logging.WARNING:
        logger.warning(logs)
    elif level == logging.INFO:
        logger.info(logs)
    else:
        logger.debug(logs)
