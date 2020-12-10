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

from dotenv import load_dotenv
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
    cache_key = f"_{key}"

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

        if not hasattr(config, cache_key):
            # Go out and get the associated value.
            try:
                value = os.environ[key]
            except KeyError as e:
                found = False

                if fetch:
                    # Go even further and fetch the value from the configuration database.
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

            # Cache the result.
            setattr(config, cache_key, value)
        else:
            # Fetch a cached result.
            value = getattr(config, cache_key)

        return value

    return _getter


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

        STRIPE_SECRET = property(getter("STRIPE_RESTRICTED"))
        TESTING = True

    return TestConfig


class DeploymentConfig:
    """Flask application configuration for deployed applications.

    "Deployed applications" are those running on deployment platforms such as Heroku. They must be
    started with CONFIG_DB_URL, DATABASE_URL, and REDIS_URL set in the process's execution
    environment.
    """

    def __init__(self):
        engine = create_engine(os.environ["CONFIG_DB_URL"], echo=False, pool_pre_ping=True)

        self.session = Session(bind=engine)

    DASHBOARD_PASSWORD = property(getter("DASHBOARD_PASSWORD"))
    DASHBOARD_USERNAME = property(getter("DASHBOARD_USERNAME"))
    DATADOG_API_KEY = property(getter("DATADOG_API_KEY"))
    DATADOG_APP_KEY = property(getter("DATADOG_APP_KEY"))
    ENDPOINT_SECRET = property(getter("ENDPOINT_SECRET"))
    FRONTEND_URL = property(getter("FRONTEND_URL"))
    HOST_SERVICE_SECRET = property(getter("HOST_SERVICE_AND_WEBSERVER_AUTH_SECRET"))
    JWT_SECRET_KEY = property(getter("JWT_SECRET_KEY"))
    REDIS_URL = property(getter("REDIS_URL", fetch=False))
    SENDGRID_API_KEY = property(getter("SENDGRID_API_KEY"))
    SENDGRID_DEFAULT_FROM = "noreply@tryfractal.com"
    SHA_SECRET_KEY = property(getter("SHA_SECRET_KEY"))
    SILENCED_ENDPOINTS = ("/status", "/ping")
    SQLALCHEMY_DATABASE_URI = property(getter("DATABASE_URL", fetch=False))
    SQLALCHEMY_TRACK_MODIFICATIONS = False
    STRIPE_SECRET = property(getter("STRIPE_SECRET"))

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


class LocalConfig(DeploymentConfig):
    """Application configuration for applications running on local development machines.

    Applications running locally must set CONFIG_DB_URL and POSTGRES_PASSWORD either in the
    process's execution environment or in a .env file.
    """

    def __init__(self):
        load_dotenv(verbose=True)
        super().__init__()

    config_table = "dev"

    # When deploying the web server locally, a developer may want to connect to a local Postgres
    # instance. The following lines allow developers to specify individual parts of the connection
    # URI and to fill in the remaining values with reasonable defaults rather than requiring them
    # to specify the whole thing.
    db_host = property(getter("POSTGRES_HOST", default="localhost", fetch=False))
    db_name = property(getter("POSTGRES_DB", default="postgres", fetch=False))
    db_password = property(getter("POSTGRES_PASSWORD", fetch=False))
    db_port = property(getter("POSTGRES_PORT", default=5432, fetch=False))
    db_user = property(getter("POSTGRES_USER", default="postgres", fetch=False))

    REDIS_URL = property(getter("REDIS_URL", default="redis://", fetch=False))
    STRIPE_SECRET = property(getter("STRIPE_RESTRICTED"))

    @property
    def SQLALCHEMY_DATABASE_URI(self):  # pylint: disable=invalid-name
        return (
            f"postgresql://{self.db_user}:{self.db_password}@{self.db_host}:{self.db_port}/"
            f"{self.db_name}"
        )


DeploymentTestConfig = _TestConfig(DeploymentConfig)
LocalTestConfig = _TestConfig(LocalConfig)

CONFIG_MATRIX = _ConfigMatrix(
    _ConfigVector(DeploymentConfig, DeploymentTestConfig),
    _ConfigVector(LocalConfig, LocalTestConfig),
)
