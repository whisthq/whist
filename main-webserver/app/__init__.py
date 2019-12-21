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
app = create_app(celery = celery)
app.config['MAIL_SERVER'] = "ming@fractalcomputers.com"
app.config['MAIL_PORT'] = 465
app.config['MAIL_USE_SSL'] = True
CORS(app)
