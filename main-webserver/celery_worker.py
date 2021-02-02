from app import celery_instance
from app.factory import create_app, register_blueprints
from app.celery_utils import init_celery


app = create_app()
init_celery(celery_instance, app)
