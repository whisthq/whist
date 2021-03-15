"""Deployable Flask application instance.

The sole purpose of this file is to contain the Flask instance that will be run when
the web server is deployed. No other modules should import this one in order to prevent deployable
application instances from being created when they are not needed (e.g. during testing).

The `flask` command will not detect the Flask application instance in this file by default. Make
sure that the environment variable `FLASK_ENV` is set such that `flask` runs the application
instantiated in this module (e.g. `FLASK_ENV=entry`). Read more about Flask application discovery
here: https://flask.palletsprojects.com/en/1.1.x/cli/?highlight=cli#application-discovery
"""

from app.factory import create_app
from app.celery_utils import make_celery
from app import set_web_requests_status
from app.helpers.utils.general.logs import fractal_logger
from app.signals import SignalHandler
from app.maintenance.maintenance_manager import maintenance_init_redis_conn

app = create_app()
celery = make_celery(app)

celery.set_default()

# enable web requests
if not set_web_requests_status(True):
    fractal_logger.fatal("Could not enable web requests at startup. Failing out.")

# enable the web signal handler
SignalHandler()

# initialize redis connection for maintenance package
maintenance_init_redis_conn(app.config["REDIS_URL"])
