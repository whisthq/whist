from .imports import *

def sendLog(log):
	# class ContextFilter(logging.Filter):
	#     hostname = socket.gethostname()

	#     def filter(self, record):
	#         record.hostname = ContextFilter.hostname
	#         return True

	# syslog = SysLogHandler(address=('logs3.papertrailapp.com', 44138))
	# syslog.addFilter(ContextFilter())

	# format = '%(asctime)s %(hostname)s YOUR_APP: %(message)s'
	# formatter = logging.Formatter(format, datefmt='%b %d %H:%M:%S')
	# syslog.setFormatter(formatter)

	# log = logging.getLogger('werkzeug')
	# log.setLevel(logging.CRITICAL)
	# log.disabled = True

	# logger = logging.getLogger()
	# logger.addHandler(syslog)
	# logger.setLevel(logging.WARNING)
	# logger.info(log)
	return 1
