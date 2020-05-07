from .imports import *

class ContextFilter(logging.Filter):
    hostname = socket.gethostname()

    def filter(self, record):
        record.hostname = ContextFilter.hostname
        return True

syslog = SysLogHandler(address=(os.getenv('PAPERTRAIL_URL'), int(os.getenv('PAPERTRAIL_PORT'))))
syslog.addFilter(ContextFilter())

format = '%(asctime)s %(hostname)s YOUR_APP: %(message)s'
formatter = logging.Formatter(format, datefmt='%b %d %H:%M:%S')
syslog.setFormatter(formatter)

logger = logging.getLogger()
logger.addHandler(syslog)
logger.setLevel(logging.INFO)

def sendInfo(ID, log, papertrail = True):
	if papertrail:
		logger.info('[WEBSERVER][{}] INFO: {}'.format(ID, log))
	print('[WEBSERVER][{}] INFO: {}'.format(ID, log))

def sendWarning(ID, log, papertrail = True):
	if papertrail:
		logger.warning('[WEBSERVER][{}] WARNING: {}'.format(ID, log))
	print('[WEBSERVER][{}] WARNING: {}'.format(ID, log))

def sendError(ID, log, papertrail = True):
	if papertrail:
		logger.error('[WEBSERVER][{}] ERROR: {}'.format(ID, log))
	print('[WEBSERVER][{}] ERROR: {}'.format(ID, log))

def sendCritical(ID, log, papertrail = True):
	if papertrail:
		logger.critical('[WEBSERVER][{}] CRITICAL: {}'.format(ID, log))
	print('[WEBSERVER][{}] CRITICAL: {}'.format(ID, log))