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

import json
import os

from collections import namedtuple

from dotenv import load_dotenv
from flask import request
from sqlalchemy import create_engine
from sqlalchemy.orm.session import Session

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
    container deletion request when the protocol terminates.


    If we're launching a streamed application from an instance of the web server running on a local
    development machine, the server is unable to receive pings from the protocol running in the
    container from which the application is being streamed. Instead, we want to direct all of the
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
    started with CONFIG_DB_URL, DATABASE_URL, and REDIS_URL or REDIS_TLS_URL set in the process's
    execution environment.
    """

    def __init__(self):
        engine = create_engine(os.environ["CONFIG_DB_URL"], echo=False, pool_pre_ping=True)

        self.session = Session(bind=engine)

    DASHBOARD_PASSWORD = property(getter("DASHBOARD_PASSWORD"))
    DASHBOARD_USERNAME = property(getter("DASHBOARD_USERNAME"))
    DATADOG_API_KEY = property(getter("DATADOG_API_KEY"))
    DATADOG_APP_KEY = property(getter("DATADOG_APP_KEY"))
    DROPBOX_APP_KEY = property(getter("DROPBOX_APP_KEY", fetch=False, raising=False))
    DROPBOX_APP_SECRET = property(getter("DROPBOX_APP_SECRET", fetch=False, raising=False))
    DROPBOX_CSRF_TOKEN_SESSION_KEY = "dropbox-auth-csrf-token"
    ENDPOINT_SECRET = property(getter("ENDPOINT_SECRET"))
    FRONTEND_URL = property(getter("FRONTEND_URL"))
    HIREFIRE_TOKEN = property(getter("HIREFIRE_TOKEN"))
    HOST_SERVICE_PORT = 4678
    HOST_SERVICE_SECRET = property(getter("HOST_SERVICE_AND_WEBSERVER_AUTH_SECRET"))
    JWT_QUERY_STRING_NAME = "access_token"
    JWT_SECRET_KEY = property(getter("JWT_SECRET_KEY"))
    JWT_TOKEN_LOCATION = ("headers", "query_string")
    SECRET_KEY = property(getter("SECRET_KEY", fetch=False))
    SENDGRID_API_KEY = property(getter("SENDGRID_API_KEY"))
    SENDGRID_DEFAULT_FROM = "noreply@fractal.co"
    SHA_SECRET_KEY = property(getter("SHA_SECRET_KEY"))
    SILENCED_ENDPOINTS = ("/status", "/ping")
    SQLALCHEMY_DATABASE_URI = property(getter("DATABASE_URL", fetch=False))
    SQLALCHEMY_TRACK_MODIFICATIONS = False
    STRIPE_SECRET = property(getter("STRIPE_SECRET"))
    AWS_TASKS_PER_INSTANCE = property(getter("AWS_TASKS_PER_INSTANCE"))
    SENTRY_DSN = property(getter("SENTRY_DSN", fetch=False))

    @property
    def config_table(self):
        """Determine which config database table fallback configuration values should be read.

        Returns:
            Either "dev", "production", or "staging".
        """

        app_name = os.environ.get("HEROKU_APP_NAME")

        if app_name == "fractal-prod-server":
            table = "production"
        elif app_name == "fractal-staging-server":
            table = "staging"
        else:
            table = "dev"

        return table

    @property
    def GOOGLE_CLIENT_SECRET_OBJECT(self):  # pylint: disable=invalid-name
        """Load the client secret configuration object from client_secret.json

        Returns:
            A Google client secret-formatted dictionary.
        """

        with open("client_secret.json") as secret_file:
            return json.loads(secret_file.read())

    @property
    def REDIS_URL(self):  # pylint: disable=invalid-name
        """Select the most secure Redis connection URI from the program's environment.

        Heroku dynos contain up to two environment variables containing Redis connection strings.
        REDIS_URL should always be present, and REDIS_TLS_URL will sometimes be present as well.

        Returns:
            A Redis connection URI as a string.
        """

        return os.environ.get("REDIS_TLS_URL") or os.environ["REDIS_URL"]


class LocalConfig(DeploymentConfig):
    """Application configuration for applications running on local development machines.

    Applications running locally must set CONFIG_DB_URL and POSTGRES_PASSWORD either in the
    process's execution environment or in a .env file.
    """

    def __init__(self):
        load_dotenv(dotenv_path=os.path.join(os.getcwd(), "docker/.env"), verbose=True)
        super().__init__()

    config_table = "dev"
    STRIPE_SECRET = property(getter("STRIPE_RESTRICTED"))
    AWS_TASKS_PER_INSTANCE = property(getter("AWS_TASKS_PER_INSTANCE", default=10, fetch=False))
    MAILSLURP_API_KEY = property(
        getter(
            "MAILSLURP_API_KEY",
            default="ade68d47a8ba39c57a8e8358e5e86d6d99a04cf8aeebf9c11c08f851f2fa438f",
            fetch=False,
        )
    )

    @property
    def GOOGLE_CLIENT_SECRET_OBJECT(self):  # pylint: disable=invalid-name
        """Load the client secret configuration from client_secret.json if the file exists.

        If client_secret.json does not exist, return an empty dictionary.

        Returns:
            A dictionary.
        """

        try:
            secret = super().GOOGLE_CLIENT_SECRET_OBJECT
        except FileNotFoundError:
            secret = {}

        return secret


def _TestConfig(BaseConfig):  # pylint: disable=invalid-name
    """Generate a test configuration class that is a subclass of a base configuration class.

    Arguments:
        BaseConfig: A base configuration class (either DeploymentConfig or LocalConfig).

    Returns:
        A configuration class to be used to configure a Flask application for testing.
    """

    class TestConfig(BaseConfig):  # pylint: disable=invalid-name
        """Place the application in testing mode."""

        config_table = "dev"

        DROPBOX_APP_KEY = "dropbox-client-id"
        DROPBOX_APP_SECRET = "dropbox-client-secret"
        STRIPE_SECRET = property(getter("STRIPE_RESTRICTED"))

        TESTING = True

        @property
        def GOOGLE_CLIENT_SECRET_OBJECT(self):  # pylint: disable=invalid-name
            # Test deployments should not be able to act as OAuth clients.
            return {
                "web": {
                    "client_id": "google-client-id",
                    "client_secret": "google-client-secret",
                }
            }

    return TestConfig


DeploymentTestConfig = _TestConfig(DeploymentConfig)
LocalTestConfig = _TestConfig(LocalConfig)

CONFIG_MATRIX = _ConfigMatrix(
    _ConfigVector(DeploymentConfig, DeploymentTestConfig),
    _ConfigVector(LocalConfig, LocalTestConfig),
)
