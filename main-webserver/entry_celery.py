"""Deployable Celery application instance.

The sole purpose of this file is to contain the Celery instance that will be run when
the web server is deployed. No other modules should import this one in order to prevent deployable
application instances from being created when they are not needed (e.g. during testing).
"""

from app.factory import create_app
from app.celery_utils import make_celery

# this registers all the celery signal handling functions. we don't need to do anything more.
import app.signals

app = create_app()
celery = make_celery(app)

celery.set_default()
