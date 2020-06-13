from app.imports import *
import logging


class ContextFilter(logging.Filter):
    hostname = socket.gethostname()

    def filter(self, record):
        record.hostname = ContextFilter.hostname
        return True


syslog = SysLogHandler(
    address=(os.getenv("PAPERTRAIL_URL"), int(os.getenv("PAPERTRAIL_PORT")))
)
syslog.addFilter(ContextFilter())

log_format = "%(asctime)s %(levelname)-8s [%(filename)s:%(lineno)d][{} WEBSERVER] %(message)s".format(
    os.getenv("SERVER_TYPE")
)
formatter = logging.Formatter(log_format, datefmt="%b %d %H:%M:%S")
syslog.setFormatter(formatter)

logger = logging.getLogger()
logger.addHandler(syslog)
