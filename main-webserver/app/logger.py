from .imports import *

class ContextFilter(logging.Filter):
    hostname = socket.gethostname()

    def filter(self, record):
        record.hostname = ContextFilter.hostname
        return True

syslog = SysLogHandler(address=(os.getenv('PAPERTRAIL_URL'), int(os.getenv('PAPERTRAIL_PORT'))))
syslog.addFilter(ContextFilter())

format = '%(asctime)s [%(pathname)s:%(lineno)d] %(message)s'
formatter = logging.Formatter(format, datefmt='%b %d %H:%M:%S')
syslog.setFormatter(formatter)

logger = logging.getLogger()
logger.addHandler(syslog)

def sendDebug(ID, log, papertrail = True):
	logger.setLevel(logging.DEBUG)
	if papertrail:
		logger.info('[{} WEBSERVER][{}] INFO: {}'.format(os.getenv('SERVER_TYPE'), ID, log))
	print('[{} WEBSERVER][{}] INFO: {}'.format(os.getenv('SERVER_TYPE'), ID, log))

def sendInfo(ID, log, papertrail = True):
	logger.setLevel(logging.INFO)
	if papertrail:
		logger.info('[{} WEBSERVER][{}] INFO: {}'.format(os.getenv('SERVER_TYPE'), ID, log))
	print('[{} WEBSERVER][{}] INFO: {}'.format(os.getenv('SERVER_TYPE'), ID, log))

def sendWarning(ID, log, papertrail = True):
	logger.setLevel(logging.WARNING)
	if papertrail:
		logger.warning('[{} WEBSERVER][{}] WARNING: {}'.format(os.getenv('SERVER_TYPE'), ID, log))
	print('[{} WEBSERVER][{}] WARNING: {}'.format(os.getenv('SERVER_TYPE'), ID, log))

def sendError(ID, log, papertrail = True):
	logger.setLevel(logging.ERROR)
	if papertrail:
		logger.error('[{} WEBSERVER][{}] ERROR: {}'.format(os.getenv('SERVER_TYPE'), ID, log))
	print('[{} WEBSERVER][{}] ERROR: {}'.format(os.getenv('SERVER_TYPE'), ID, log))

def sendCritical(ID, log, papertrail = True):
	logger.setLevel(logging.CRITICAL)
	if papertrail:
		logger.critical('[{} WEBSERVER][{}] CRITICAL: {}'.format(os.getenv('SERVER_TYPE'), ID, log))
	print('[{} WEBSERVER][{}] CRITICAL: {}'.format(os.getenv('SERVER_TYPE'), ID, log))