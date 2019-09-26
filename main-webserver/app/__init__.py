from .imports import *
from .factory import *

def make_celery(app_name = __name__):
    backend = os.getenv('REDIS_URL')
    broker = os.getenv('REDIS_URL')
    return Celery(app_name, backend=backend, broker=broker)

celery = make_celery()
celery.conf.update(task_track_started = True)
app = create_app(celery = celery)
