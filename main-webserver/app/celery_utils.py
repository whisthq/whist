"""Celery application configuration.

Celery application instances are configured by a large mapping of configuration keys to
values. Some of those key-value pairs may be set by passing certain arguments to the Celery
constructor. Others must be set directly on the configuration mapping.

In this file, CELERY_CONFIG is a dictionary that can be used to update the underlying Celery
application configuration mapping. celery_params takes a Flask application instance as input and
returns a dictionary containing keyword arguments to pass to the Celery constructor. make_celery
is a function that actually instantiates a Celery application that the Flask application can use
to delegate long-running tasks to asynchronous worker processes.

Structuring the file in this way allows for the Celery application to be configured for testing in
the same way that it is configured for deployment. See the descriptions of the celery_config and
celery_parameters test fixtures in the official Celery documentation and their implementations in
main-webserver/tests/conftest.py.

https://docs.celeryproject.org/en/stable/userguide/testing.html#fixtures
"""

import ssl

from celery import Celery
from celery.app.task import Task
from hirefire.contrib.flask.blueprint import build_hirefire_blueprint
from hirefire.procs.celery import CeleryProc

# Celery configuration options used to configure all Celery application instances. The WORKER is
# the program that is started with
#
#     celery --app ... worker ...
#
# The BROKER and the BACKEND are the same Redis instance, but the broker stores information about
# tasks that are due to be processed and the backend stores the results of processed tasks.
CELERY_CONFIG = {
    "accept_content": ("pickle",),
    # Whether or not the worker should retry a failed connection.
    "broker_connection_retry": False,
    # See the Kombu source file containing the implementation of the Redis transport for a list of
    # valid transport options. These options configure the connection to the broker.
    # https://github.com/celery/kombu/blob/6cb9d6639e800012e733a0535db34653705290b9/kombu/transport/redis.py#L28
    "broker_transport_options": {
        "max_retries": 1,
        "socket_timeout": 1,
    },
    "redis_max_connections": 40,
    "result_accept_content": ("json",),
    # This key seems to set the socket timeout for the connection to the Redis backend (i.e.
    # result store). It is important to set the socket timeout so that a failed SSL handshake will
    # cause a TimeoutError to be raised rather than hanging.
    "redis_socket_timeout": 1,
    # These options configure the connection to the backend. Acceptable keys in the retry_policy
    # dictionary are arguments to the Kombu retry_over_time function. See
    # https://docs.celeryproject.org/en/stable/getting-started/brokers/redis.html#connection-timeouts
    "result_backend_transport_options": {
        "retry_policy": {
            "max_retries": 1,
        },
    },
    "task_serializer": "pickle",
    "task_track_started": True,
    # this stops celery from overriding our logger
    "worker_hijack_root_logger": False,
}


def celery_params(flask_app):
    """Generate the parameters to use to instantiate a Flask application's Celery instance.

    The Flask application instance passed as the only argument to this function provides the Redis
    connection string of the Redis instance to which the new Celery instance should connect and
    the Flask application context with which each Celery task should be called.

    Args:
        flask_app: The instance of the Flask application with which the new Celery instance should
            be compatible.

    Returns:
        A dictionary containing the keyword arguments that should be passed to the Celery
        constructor in order to instantiate a Celery application instance that is compatible with
        the provided Flask application.
    """

    class ContextTask(Task):  # pylint: disable=abstract-method
        """A Celery Task subclass whose run method is called with Flask application context.

        See https://flask.palletsprojects.com/en/1.1.x/patterns/celery/#configure.
        """

        def __call__(self, *args, **kwargs):
            with flask_app.app_context():
                return self.run(*args, **kwargs)

    redis_url = flask_app.config["REDIS_URL"]
    parameters = {
        "broker": redis_url,
        "backend": redis_url,
        "task_cls": ContextTask,
    }

    if redis_url.startswith("rediss"):
        parameters["broker_use_ssl"] = {"ssl_cert_reqs": ssl.CERT_NONE}
        parameters["redis_backend_use_ssl"] = {"ssl_cert_reqs": ssl.CERT_NONE}

    return parameters


def make_celery(flask_app):
    """Create the Celery application instance that accompanies the specified Flask application.

    Args:
        flask_app: The Flask application instance whose tasks the new Celery instance should
           process.

    Returns:
        A Celery application instance.
    """

    celery = Celery(flask_app.import_name, set_as_current=True, **celery_params(flask_app))

    class SimpleCeleryProc(CeleryProc):
        queues = ("celery",)
        simple_queues = True

    worker_proc = SimpleCeleryProc(name="celery", app=celery)
    hirefire_bp = build_hirefire_blueprint(flask_app.config["HIREFIRE_TOKEN"], (worker_proc,))

    celery.conf.update(CELERY_CONFIG)
    flask_app.register_blueprint(hirefire_bp)

    return celery
