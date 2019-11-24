from .imports import *
from .factory import *
from celery.signals import worker_process_init

@worker_process_init.connect
def on_fork_close_session(**kwargs):
    if db_session is not None:
        db_session.close()
        db_engine.dispose()

def make_celery(app_name = __name__):
    backend = os.getenv('REDIS_URL')
    broker = os.getenv('REDIS_URL')
    return Celery(app_name, backend=backend, broker=broker)


celery = make_celery()
engine = db.create_engine(
    os.getenv('DATABASE_URL'), echo=True, pool_pre_ping = True)
conn = engine.connect()
app = create_app(celery = celery)
CORS(app)
