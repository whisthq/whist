"""Deployable Flask application instance.

The sole purpose of this file is to contain the Flask instance that will be run when
the web server is deployed. No other modules should import this one in order to prevent deployable
application instances from being created when they are not needed (e.g. during testing).

The `flask` command will not detect the Flask application instance in this file by default. Make
sure that the environment variable `FLASK_ENV` is set such that `flask` runs the application
instantiated in this module (e.g. `FLASK_ENV=entry`). Read more about Flask application discovery
here: https://flask.palletsprojects.com/en/1.1.x/cli/?highlight=cli#application-discovery
"""
import os
import platform

from app.factory import create_app
from app.flask_handlers import set_web_requests_status
from app.helpers.utils.general.logs import fractal_logger
from app.signals import WebSignalHandler

# if testing, TESTING env var should be set. Default is False
is_testing = os.environ.get("TESTING", "False") in ("True", "true")
app = create_app(testing=is_testing)

# enable web requests
if not set_web_requests_status(True):
    fractal_logger.fatal("Could not enable web requests at startup. Failing out.")
    os.sys.exit(1)

# enable the web signal handler. This should work on OSX and Linux.
if "windows" in platform.platform().lower():
    fractal_logger.warning("signal handler is not supported on windows. skipping enabling them.")
else:
    WebSignalHandler()
