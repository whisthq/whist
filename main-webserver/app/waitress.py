
from app import make_celery, create_app

celery_instance = make_celery()

celery_instance.set_default()

app = create_app(celery=celery_instance)
