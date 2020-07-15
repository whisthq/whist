from app import *
from app.factory import create_app, init_app
from app.celery_utils import init_celery

app, jwtManager = create_app()
app = init_app(app)
init_celery(celery_instance, app)
