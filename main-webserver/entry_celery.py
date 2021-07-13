"""Deployable Celery application instance.

The sole purpose of this file is to contain the Celery instance that will be run when
the web server is deployed. No other modules should import this one in order to prevent deployable
application instances from being created when they are not needed (e.g. during testing).
"""

import os

from app.factory import create_app
from app.celery_utils import make_celery
from app.maintenance.maintenance_manager import maintenance_init_redis_conn

# this registers all the celery signal handling functions. we don't need to do anything more.
import app.signals

# if testing, TESTING env var should be set. Default is False
is_testing = os.environ.get("TESTING", "False") in ("True", "true")
app = create_app(testing=is_testing)
celery = make_celery(app)

celery.set_default()

# initialize redis connection for maintenance package
maintenance_init_redis_conn(app.config["REDIS_URL"])  # type: ignore[index]
