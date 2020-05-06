from .imports import *
from .factory import *

def make_celery(app_name = __name__):
	backend = os.getenv('REDIS_URL')
	broker = os.getenv('REDIS_URL')
	return Celery(app_name, backend=backend, broker=broker)

def dispose_engine(engine):
	engine.dispose()

celery = make_celery()
engine = db.create_engine(
	os.getenv('DATABASE_URL'), echo=True, pool_pre_ping = True)
register_after_fork(engine, dispose_engine)
app, jwtManager = create_app(celery = celery)
app = init_app(app)
# celery.conf.update(enable_utc=True, timezone='US_Eastern')
# celery.conf['CELERY_TIMEZONE'] = 'US/Eastern'

app.config['MAIL_SERVER'] = "ming@fractalcomputers.com"
app.config['MAIL_PORT'] = 465
app.config['MAIL_USE_SSL'] = True
app.config['JWT_SECRET_KEY'] = os.getenv('JWT_SECRET_KEY')

CORS(app)
