from app import make_celery
from app.factory import create_app
from app.celery_utils import init_celery

# same as app/waitress.py, but can't be shared as hirefire thinks
# it is initializing twice and overlapping workers
celery_instance = make_celery()
celery_instance.set_default()
app = create_app(celery=celery_instance)
init_celery(celery_instance, app)
