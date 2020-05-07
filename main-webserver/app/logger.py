from .imports import *

class ContextFilter(logging.Filter):
    hostname = socket.gethostname()

    def filter(self, record):
        record.hostname = ContextFilter.hostname
        return True

syslog = SysLogHandler(address=('logs3.papertrailapp.com', 44138))
syslog.addFilter(ContextFilter())

format = '%(asctime)s %(hostname)s YOUR_APP: %(message)s'
formatter = logging.Formatter(format, datefmt='%b %d %H:%M:%S')
syslog.setFormatter(formatter)

logger = logging.getLogger()
logger.addHandler(syslog)
logger.setLevel(logging.INFO)

def sendInfo(ID, log, papertrail = True):
	if papertrail:
		logger.info('{} INFO: {}'.format(ID, log))
	print('{} [HIDDEN] INFO: {}'.format(ID, log))

def sendError(ID, log, papertrail = True):
	if papertrail:
		logger.error('{} ERROR: {}'.format(ID, log))
	print('{} [HIDDEN] ERROR: {}'.format(ID, log))

def sendCritical(ID, log, papertrail = True):
	if papertrail:
		logger.critical('{} CRITICAL: {}'.format(ID, log))
	print('{} [HIDDEN] CRITICAL: {}'.format(ID, log))
