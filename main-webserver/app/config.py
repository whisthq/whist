"""Flask application configuration.

This module contains definitions of objects that are used to configure the Flask application a la
https://flask.palletsprojects.com/en/1.1.x/config/#development-production. Depending on the
environment in which the Flask application is being deployed, one of these configuration objects is
selected to be used to configure the application. For example, if the application factory detects
that the application is being deployed during a local test session, LocalTestConfig will be
selected; if the application factory detects that the application is being deployed to production
on Heroku, the DeploymentConfig class will be selected. The configuration object is then
instantiated and the Flask application is configured by flask.Config.from_object().
"""

import os

from collections import namedtuple
from urllib.parse import urlsplit, urlunsplit

from dotenv import load_dotenv
from flask import request
from sqlalchemy import create_engine
from sqlalchemy.orm.session import Session

from app.constants import config_table_names, env_names

# A _ConfigMatrix instance is a two-dimensional object that resembles a 2x2 matrix and is used to
# look up Flask application configuration objects. The first dimension maps the strings "deployed"
# and "local" to _ConfigVector instances, which in turn map the strings "serve" and "test" to
# configuration objects on the second dimension. Note that object syntax, as opposed to dictionary
# syntax is used to look up strings in the matrix. For example, CONFIG_MATRIX.deployed.serve
# evaluates to DeployedConfig and CONFIG_MATRIX.local.test evaluates to LocalTestConfig.
_ConfigMatrix = namedtuple("_ConfigMatrix", ("deployment", "local"))
_ConfigVector = namedtuple("_ConfigVector", ("serve", "test"))


def _callback_webserver_hostname():
    """Return the hostname of the web server with which the protocol server should communicate.

    The callback web server will receive pings from the protocol server and will receive the
    mandelbox deletion request when the protocol terminates.


    If we're launching a streamed application from an instance of the web server running on a local
    development machine, the server is unable to receive pings from the protocol running in the
    mandelbox from which the application is being streamed. Instead, we want to direct all of the
    protocol's communication to the Fractal development server deployment on Heroku.

    This function must be called with request context.

    Returns:
        A web server hostname.
    """

    return (
        request.host
        if not any((host in request.host for host in ("localhost", "127.0.0.1")))
        else "dev-server.fractal.co"
    )


def _ensure_postgresql(conn_string: str) -> str:
    """Ensure that a particular PostgreSQL connection string's scheme is "postgresql".

    PostgreSQL connection strings that start with "postgres://" have long been deprecated and are
    no longer supported since SQLAlchemy 1.4.0b1. Unfortunately, the PostgreSQL connection strings
    with which Heroku automatically populates environment variables all begin with "postgres://"
    (see for yourself by running heroku config:get DATABASE_URL --app=fractal-dev-server).

    https://docs.sqlalchemy.org/en/14/changelog/changelog_14.html#change-3687655465c25a39b968b4f5f6e9170b

    Args:
        conn_string: A PostgreSQL connection string. It should start with either "postgres://" or
            "postgresql://".

    Returns:
        The same PostgreSQL connection string whose scheme is guaranteed to be "postgresql".
    """

    _scheme, *parts = urlsplit(conn_string)

    return urlunsplit(("postgresql", *parts))


def getter(key, fetch=True, **kwargs):
    """Return a getter function that can be passed to the builtin property decorator.

    This function attempts to retrieve the value associated with a configuration variable key. It
    first tries to retrieve the value of the environment variable whose name matches the key. If
    no environment variable is set in the process's execution environment it may attempt to fetch
    the value associated with the specified key from Fractal's configuration database. If the key
    cannot be found, a default value is returned or a KeyError is raised.

    Arguments:
        key: The key to retrieve.
        fetch: A boolean indicating whether or not to fetch this key from the configuration
            database.

    Keyword Arguments:
        default: An optional default value to return if the key is not found. Setting implies
            raising=False.
        raising: A boolean indicating whether or not to raise a KeyError if a key cannot be
            found.

    Returns:
        A getter function that retrieves the value associated with the specified key from the
        process's execution environment, optionally falling back on the configuration database.
    """

    default = kwargs.get("default")
    has_default = "default" in kwargs
    raising = kwargs.get("raising", not has_default)

    def _getter(config):
        """Return an instance attribute value.

        This function is an instance method suitable to be passed as the first argument to the
        built-in property() decorator. Were this method defined as instance methods are normally
        defined, it would be clear that the config argument is actually the instance on which the
        method is defined, i.e. self.

        Arguments:
            config: The instance on which the method is defined. Passed automatically when this
                function is called using instance.method() syntax.

        Returns:
            Either a str or an int; the value of the configuration variable for which this function
                has been generated to be a getter method.

        Raises:
            KeyError: The configuration key for which this function has been generated to be a
                getter method could be found neither in the process's execution environment nor in
                the configuration database.
        """

        try:
            # First, try to read the value of the configuration variable from the process's
            # execution environment.
            value = os.environ[key]
        except KeyError as e:
            found = False

            if fetch:
                # Attempt to read a fallback value from the configuration database.
                result = config.session.execute(
                    f"SELECT value FROM {config.config_table} WHERE key=:key",
                    {"key": key},
                )
                row = result.fetchone()

                if row:
                    found = True
                    value = row[0]

                    result.close()

            if not found:
                if not has_default and raising:
                    raise e

                value = default

        return value

    return _getter


class DeploymentConfig:
    """Flask application configuration for deployed applications.

    "Deployed applications" are those running on deployment platforms such as Heroku. They must be
    started with CONFIG_DB_URL, DATABASE_URL set in the process's
    execution environment.
    """

    def __init__(self):
        engine = create_engine(
            _ensure_postgresql(os.environ["CONFIG_DB_URL"]), echo=False, pool_pre_ping=True
        )

        self.session = Session(bind=engine)

    database_url = property(getter("DATABASE_URL", fetch=False))

    AUTH0_DOMAIN = property(getter("AUTH0_DOMAIN"))
    AUTH0_WEBSERVER_CLIENT_ID = property(getter("AUTH0_WEBSERVER_CLIENT_ID"))
    AUTH0_WEBSERVER_CLIENT_SECRET = property(getter("AUTH0_WEBSERVER_CLIENT_SECRET"))
    RUNNING_LOCALLY = property(getter("RUNNING_LOCALLY", default=False))
    ENDPOINT_SECRET = property(getter("ENDPOINT_SECRET"))
    FRONTEND_URL = property(getter("FRONTEND_URL"))
    HOST_SERVICE_PORT = property(getter("HOST_SERVICE_PORT", default="4678"))
    JWT_ALGORITHM = "RS256"
    JWT_DECODE_ALGORITHMS = ("RS256",)
    JWT_DECODE_AUDIENCE = "https://api.fractal.co"
    JWT_QUERY_STRING_NAME = "access_token"
    JWT_TOKEN_LOCATION = ("headers", "query_string")
    SENDGRID_API_KEY = property(getter("SENDGRID_API_KEY"))
    SENDGRID_DEFAULT_FROM = "noreply@fractal.co"
    SILENCED_ENDPOINTS = ("/status", "/ping")
    SQLALCHEMY_TRACK_MODIFICATIONS = False
    STRIPE_CUSTOMER_ID_CLAIM = "https://api.fractal.co/stripe_customer_id"
    STRIPE_SECRET = property(getter("STRIPE_SECRET"))
    STRIPE_PRICE_ID = property(getter("STRIPE_PRICE_ID"))
    AWS_TASKS_PER_INSTANCE = property(getter("AWS_TASKS_PER_INSTANCE"))
    AWS_INSTANCE_TYPE_TO_LAUNCH = property(
        # Having a `fetch=True` can let us dynamically change the instance type to be launched.
        getter("AWS_INSTANCE_TYPE_TO_LAUNCH", fetch=True, default="g4dn.12xlarge")
    )
    DESIRED_FREE_MANDELBOXES = property(
        getter("DESIRED_FREE_MANDELBOXES", fetch=True, default=20)
    )
    DEFAULT_INSTANCE_BUFFER = property(
        # This will be as a count to launch new instances when we don't have
        #  any instances with the current AMI running.
        getter("DEFAULT_INSTANCE_BUFFER", fetch=True, default=1)
    )
    SENTRY_DSN = property(getter("SENTRY_DSN", fetch=False))

    # If this is not set then metrics will not be shipped to logz
    METRICS_SINK_LOGZ_TOKEN = property(getter("METRICS_SINK_LOGZ_TOKEN", raising=False))

    # Set this to append metrics' JSON to a local file; intended for development + testing
    METRICS_SINK_LOCAL_FILE = property(
        getter("METRICS_SINK_LOCAL_FILE", fetch=False, raising=False)
    )

    @property
    def ENVIRONMENT(self) -> str:  # pylint: disable=invalid-name
        """Which environment (production, staging, development, local) is the application
        running in?

        Many other values are derived off of this, such as which task_definitions to use.

        NOTE: As a fallback, the "ENVIRONMENT" key can set to an environment name to
        override the automatically calculated environment. If set, the value of ENVIRONMENT
        will be returned as is.

        NOTE: This assumes the app is running on Heroku. Local configs should override
        this property. If a heroku environment is not being used, this method will fail
        (unless ENVIRONMENT is set).

            * If Heroku is being used, but the app name is unrecognized, this will default to
            development since it's assumed that a review app is being used (which is effectively a
            development deployment).

            * If Heroku is being used, and the app is in a test environment, then development
            will be returned as the environment.

        Returns:
            @see app.constants.env_names
        """

        if (override_env := os.environ.get("ENVIRONMENT")) is not None:
            return override_env

        if "HEROKU_TEST_RUN_ID" in os.environ:
            return env_names.TESTING

        heroku_app_name = os.environ["HEROKU_APP_NAME"]
        if heroku_app_name == "fractal-prod-server":
            return env_names.PRODUCTION
        elif heroku_app_name == "fractal-staging-server":
            return env_names.STAGING

        return env_names.DEVELOPMENT

    @property
    def config_table(self):
        """Determine which config database table fallback configuration values should be read.

        Returns:
            @see app.constants.config_table_names
        """
        return config_table_names.from_env(self.ENVIRONMENT)

    @property
    def APP_GIT_BRANCH(self):  # pylint: disable=invalid-name
        """The git branch (i.e. sha) of the source code for this particular instance of the
        application.

        The current implementation assumes that Heroku is the execution environment. It should fail
        if this is not the case (hence os.environ[] instead of os.environ.get()). It supports
        standard heroku app environments as well as test environments.
        """
        if (override_branch := os.environ.get("APP_GIT_BRANCH")) is not None:
            return override_branch

        # if running a review app
        if (review_branch := os.environ.get("HEROKU_BRANCH")) is not None:
            return review_branch

        # if running in a heroku test environment, such as via `heroku ci:run`
        if (test_branch := os.environ.get("HEROKU_TEST_RUN_BRANCH")) is not None:
            return test_branch

        return os.environ[
            "BRANCH"
        ]  # requires each environment, dev, staging, and prod to have a branch variable

    @property
    def APP_GIT_COMMIT(self):  # pylint: disable=invalid-name
        """The git commit (i.e. sha) of the source code for this particular instance of the
        application.

        The current implementation assumes that Heroku is the execution environment. It should fail
        if this is not the case (hence os.environ[] instead of os.environ.get()). It supports
        standard heroku app environments as well as test environments.
        """

        # If need be, APP_GIT_COMMIT can be set in the environment to override any automated commit
        # extraction from the environment. This is not recommended, though it's left as an escape
        # hatch if the app is being run on a non-Heroku platform for dev purposes and it's not yet
        # worth changing the source code to correctly extract the git commit information.
        if (override_commit := os.environ.get("APP_GIT_COMMIT")) is not None:
            return override_commit

        # if running in a heroku test environment, such as via `heroku ci:run`
        if (test_commit := os.environ.get("HEROKU_TEST_RUN_COMMIT_VERSION")) is not None:
            return test_commit

        return os.environ["HEROKU_SLUG_COMMIT"]  # requires heroku lab's runtime-dyno-metadata

    @property
    def HOST_SERVER(self):  # pylint: disable=invalid-name
        """A unique identifier representing the server (ie. VM) that this instance of the
        application is running on.

        The current implementation assumes that Heroku is the execution environment. It should fail
        if this is not the case (hence os.environ[] instead of os.environ.get()).
        """

        # If need be, HOST_SERVER can be set in the environment to override any automated server ID
        # extraction from the environment. This is not recommended, though it's left as an escape
        # hatch if the app is being run on a non-Heroku platform for dev purposes and it's not yet
        # worth changing the source code to correctly extract the host server identifiers.
        if (override_host := os.environ.get("HOST_SERVER")) is not None:
            return override_host

        if (test_dyno := os.environ.get("DYNO")) is not None:
            return test_dyno

        return "heroku-" + os.environ["HEROKU_DYNO_ID"]

    @property
    def SQLALCHEMY_DATABASE_URI(self):  # pylint: disable=invalid-name
        """The connection string of the application's main database.

        Returns:
            A PostgreSQL connection string whose scheme is guaranteed to be "postgresql".
        """

        return _ensure_postgresql(self.database_url)


class LocalConfig(DeploymentConfig):
    """Application configuration for applications running on local development machines.

    Applications running locally must set CONFIG_DB_URL and POSTGRES_PASSWORD either in the
    process's execution environment or in a .env file.
    """

    def __init__(self):
        load_dotenv(dotenv_path=os.path.join(os.getcwd(), "docker/.env"), verbose=True)
        super().__init__()

    # TODO remove type: ignore once resolved -> https://github.com/python/mypy/issues/4125
    ENVIRONMENT = property(getter("ENVIRONMENT", fetch=False, default=env_names.LOCAL))  # type: ignore # pylint: disable=line-too-long
    APP_GIT_BRANCH = property(getter("APP_GIT_BRANCH", default="unknown", fetch=False))
    APP_GIT_COMMIT = property(getter("APP_GIT_COMMIT", default="unknown", fetch=False))
    HOST_SERVER = "local-unknown"
    AUTH0_DOMAIN = property(getter("AUTH0_DOMAIN"))
    AUTH0_WEBSERVER_CLIENT_ID = property(getter("AUTH0_WEBSERVER_CLIENT_ID"))
    AUTH0_WEBSERVER_CLIENT_SECRET = property(getter("AUTH0_WEBSERVER_CLIENT_SECRET"))

    STRIPE_SECRET = property(getter("STRIPE_RESTRICTED"))
    AWS_TASKS_PER_INSTANCE = property(getter("AWS_TASKS_PER_INSTANCE", default=10, fetch=False))
    # TODO remove type: ignore once resolved -> https://github.com/python/mypy/issues/4125
    SENTRY_DSN = ""  # type: ignore


def _TestConfig(BaseConfig):  # pylint: disable=invalid-name
    """Generate a test configuration class that is a subclass of a base configuration class.

    Arguments:
        BaseConfig: A base configuration class (either DeploymentConfig or LocalConfig).

    Returns:
        A configuration class to be used to configure a Flask application for testing.
    """

    class TestConfig(BaseConfig):  # pylint: disable=invalid-name
        """Place the application in testing mode."""

        config_table = config_table_names.DEVELOPMENT

        AUTH0_DOMAIN = None
        JWT_DECODE_AUDIENCE = None
        JWT_ALGORITHM = "HS256"
        JWT_DECODE_ALGORITHMS = ("HS256",)
        JWT_SECRET_KEY = "secret"
        STRIPE_SECRET = property(getter("STRIPE_RESTRICTED"))
        SENTRY_DSN = ""

        TESTING = True

    return TestConfig


DeploymentTestConfig = _TestConfig(DeploymentConfig)
LocalTestConfig = _TestConfig(LocalConfig)

CONFIG_MATRIX = _ConfigMatrix(
    _ConfigVector(DeploymentConfig, DeploymentTestConfig),
    _ConfigVector(LocalConfig, LocalTestConfig),
)
