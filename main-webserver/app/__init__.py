from .imports import *
from .factory import *
from sqlalchemy import create_engine, MetaData
from sqlalchemy.orm import scoped_session, sessionmaker

def make_celery(app_name = __name__):
    backend = os.getenv('REDIS_URL')
    broker = os.getenv('REDIS_URL')
    return Celery(app_name, backend=backend, broker=broker)

celery = make_celery()
app = create_app(celery = celery)
CORS(app)


