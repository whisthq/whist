from .imports import *
from .factory import *

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
