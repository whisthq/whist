from .imports import *

# class MyLogger(logging.Logger):
#     def __init__(self, name=__name__, level=logging.INFO):
#         return super(MyLogger, self).__init__(name, level)

#     def debug(self, ID, msg, papertrail=True, *args, **kwargs):
#         return super(MyLogger, self).debug(
#             "[{} WEBSERVER][{}]: {}".format(os.getenv("SERVER_TYPE")), *args, **kwargs
#         )

#     def info(self, ID, msg, papertrail=True, *args, **kwargs):
#         return super(MyLogger, self).info(
#             "[{} WEBSERVER][{}]: {}".format(os.getenv("SERVER_TYPE")), *args, **kwargs
#         )

#     def warning(self, ID, msg, papertrail=True, *args, **kwargs):
#         return super(MyLogger, self).warning(
#             "[{} WEBSERVER][{}]: {}".format(os.getenv("SERVER_TYPE")), *args, **kwargs
#         )

#     def error(self, ID, msg, papertrail=True, *args, **kwargs):
#         return super(MyLogger, self).error(
#             "[{} WEBSERVER][{}]: {}".format(os.getenv("SERVER_TYPE")), *args, **kwargs
#         )

#     def critical(self, ID, msg, papertrail=True, *args, **kwargs):
#         return super(MyLogger, self).critical(
#             "[{} WEBSERVER][{}]: {}".format(os.getenv("SERVER_TYPE")), *args, **kwargs
#         )


# class ContextFilter(logging.Filter):
# 	hostname = socket.gethostname()

# 	def filter(self, record):
# 		record.hostname = ContextFilter.hostname
# 		return True


# syslog = SysLogHandler(
# 	address=(os.getenv("PAPERTRAIL_URL"), int(os.getenv("PAPERTRAIL_PORT")))
# )
# syslog.addFilter(ContextFilter())

# format = "%(asctime)s %(levelname)-8s [%(filename)s:%(lineno)d] %(message)s"
# # format = "%(asctime)s [%(pathname)s:%(lineno)d] %(message)s"
# formatter = logging.Formatter(format, datefmt="%b %d %H:%M:%S")
# syslog.setFormatter(formatter)
# # logging.setLoggerClass(MyLogger)


# logger = logging.getLogger()
# # Sets the minimum logging priority to actually log (DEBUG < INFO < WARNING < ERROR < CRITICAL)
# logger.setLevel(logging.DEBUG)
# logger.addHandler(syslog)


def sendDebug(ID, log, papertrail=True):
    # print("[{} WEBSERVER][{}]: {}".format(os.getenv("SERVER_TYPE"), ID, log))
    # if papertrail:
    #     logger.debug("[{} WEBSERVER][{}]: {}".format(os.getenv("SERVER_TYPE"), ID, log))
    return


def sendInfo(ID, log, papertrail=True):
    print("Sending info: {}".format(log))
    # if papertrail:
    #     logger.info("[{} WEBSERVER][{}]: {}".format(os.getenv("SERVER_TYPE"), ID, log))


def sendWarning(ID, log, papertrail=True):
    # print("Sending warning: {}".format(log))
    # if papertrail:
    #     logger.warning(
    #         "[{} WEBSERVER][{}]: {}".format(os.getenv("SERVER_TYPE"), ID, log)
    #     )
    return


def sendError(ID, log, papertrail=True):
    print("Sending error: {}".format(log))
    # if papertrail:
    #     logger.error("[{} WEBSERVER][{}]: {}".format(os.getenv("SERVER_TYPE"), ID, log))


def sendCritical(ID, log, papertrail=True):
    # if papertrail:
    # 	logger.critical(
    # 		"[{} WEBSERVER][{}]: {}".format(os.getenv("SERVER_TYPE"), ID, log)
    # 	)
    print(log)
