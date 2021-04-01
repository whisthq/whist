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
from celery.backends.redis import RedisBackend
from celery.loaders.base import BaseLoader
from hirefire.contrib.flask.blueprint import build_hirefire_blueprint
from hirefire.procs.celery import CeleryProc
from app.helpers.utils.metrics.celery import register_metrics_monitor_in_worker

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


class FractalLoader(BaseLoader):
    """A custom subclass of the base Celery loader class that lets us use a patched Redis backend.

    The following explanation applies to Celery 5.0.5.

    In order to use a patched version of the default Redis backend as the backend for the Fractal
    Celery application, it was necessary to reverse engineer the value of the celery_app.backend
    property.

    https://github.com/celery/celery/blob/6157bc9053da6c1b149283d0ab58fe2958a8f2dd/celery/app/base.py#L1212

    celery_app.backend is a property whose value is set by the celery_app._get_backend() method.
    celery_app._get_backend() calls celery.app.backends.by_url() It is fairly straightforward to
    reverse engineer celery.app.backends.by_url() and celery.app.backends.by_name() to determine
    that instantiating a Celery application with a custom Celery loader class on which the
    override_backends attribute is defined makes it possible to modify the default backend class
    lookup behavior. In particular, defining this custom loader class allows us to inject the
    import path of the patched Redis backend into the celery.app.backends.BACKEND_ALIASES lookup
    table.

    Attributes:
        override_backends: A mapping whose keys are nicknames for supported celery backends (e.g.
            "amqp", "rpc", "redis") and whose values are strings representing the dotted import
            paths of the Python classes that provide support for those backends.
    """

    override_backends = {"redis": "app.celery_utils:FractalRedisBackend"}


class FractalRedisBackend(RedisBackend):
    """A patched version of the default Redis Celery backend.

    The only difference between this patched Redis backend and the default version that ships with
    the Celery package is that this version will report that a task is in a special null state if
    it is not listed in the Redis store, whereas the default implementation will always fall back
    on reporting tasks as PENDING. As explained at a high level in #1508, it's impossible to
    differentiate between enqueued tasks that have not yet been picked up by a worker and
    nonexistent tasks.

    See the implementation of celery.backends.base.BaseKeyValueStoreBackend._get_task_meta_for to
    understand why the default Celery backend always falls back on reporting tasks as PENDING.

    https://github.com/celery/celery/blob/6157bc9053da6c1b149283d0ab58fe2958a8f2dd/celery/backends/base.py#L875
    """

    def get(self, key: bytes) -> bytes:
        """Retrieve the value associated with a particular task metadata key in the database.

        Note that the key is not just the UUID of the task whose metadata we want to retrieve; it
        is the concatenation of this class's task_keyprefix attribute, which is inherited from
        RedisBackend.task_keyprefix, and the the task's UUID. For all intents and purposes, this
        class's task_keyprefix attribute is a constant whose value is "celery-task-meta-".

        https://github.com/celery/celery/blob/a6bae50be937e3cf0366efac08869b39778646a5/celery/backends/base.py#L713

        Args:
            key: A byte string representing the task's metadata key (e.g. celery-task-meta-<uuid>).

        Returns:
            A byte string representing the byte encoding of a JSON object containing the metadata
            associated with the specified task metadata key in the Redis store. The encoded object
            has two keys, "status", and "result", both of whose values will be null iff the key
            cannot be found in the Redis store.
        """

        return super().get(key) or b'{"status": null, "result": null}'


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
        "loader": FractalLoader,
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

    register_metrics_monitor_in_worker(flask_app)

    class SimpleCeleryProc(CeleryProc):
        queues = ("celery",)
        simple_queues = True

    worker_proc = SimpleCeleryProc(name="celery", app=celery)
    hirefire_bp = build_hirefire_blueprint(flask_app.config["HIREFIRE_TOKEN"], (worker_proc,))

    celery.conf.update(CELERY_CONFIG)
    flask_app.register_blueprint(hirefire_bp)

    return celery
