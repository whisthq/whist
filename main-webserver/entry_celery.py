"""Deployable Celery application instance.

The sole purpose of this file is to contain the Celery instance that will be run when
the web server is deployed. No other modules should import this one in order to prevent deployable
application instances from being created when they are not needed (e.g. during testing).
"""

from app.factory import create_app
from app.celery_utils import make_celery

app = create_app()
celery = make_celery(app)

celery.set_default()

# we do not install any signal handlers for celery. It already handles SIGTERM/SIGINT
# by stopping the worker from consuming new tasks.
