from app import celery_instance, app
from app.celery_utils import init_celery

init_celery(celery_instance, app)
