from celery import Celery
import os
from dotenv import *

load_dotenv()

def make_celery(app_name = __name__):
    backend = os.getenv('REDIS_URL')
    broker = os.getenv('REDIS_URL')
    return Celery(app_name, backend=backend, broker=broker)

celery = make_celery()
