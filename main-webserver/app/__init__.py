from .imports import *
from .factory import *
from .utils import *


def make_celery(app_name = __name__):
	backend = os.getenv('REDIS_URL')
	broker = os.getenv('REDIS_URL')
	return Celery(app_name, backend=backend, broker=broker)

def dispose_engine(engine):
	engine.dispose()


celery = make_celery()
engine = db.create_engine(
	os.getenv('DATABASE_URL'), echo=False, pool_pre_ping = True)
register_after_fork(engine, dispose_engine)
app, jwtManager = create_app(celery = celery)
gen = yieldNumber()

def generateID(f):
	@wraps(f)
	def wrapper(*args, **kwargs):
		kwargs['ID'] = next(gen)
		return f(*args, **kwargs)
	return wrapper

def logRequestInfo(f):
	@wraps(f)
	def wrapper(*args, **kwargs):
		try:
			print(request.method)
			print(request.get_json())
		except Exception as e:
			print(str(e))
		return f(*args, **kwargs)
	return wrapper

app = init_app(app)
# celery.conf.update(enable_utc=True, timezone='US_Eastern')
# celery.conf['CELERY_TIMEZONE'] = 'US/Eastern'

app.config['MAIL_SERVER'] = "ming@fractalcomputers.com"
app.config['MAIL_PORT'] = 465
app.config['MAIL_USE_SSL'] = True
app.config['JWT_SECRET_KEY'] = os.getenv('JWT_SECRET_KEY')

CORS(app)
