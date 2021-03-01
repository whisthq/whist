import os
import uuid

from contextlib import contextmanager
from datetime import datetime, timezone
from random import getrandbits as randbits

import pytest

from flask_jwt_extended import create_access_token

from app.celery_utils import CELERY_CONFIG, celery_params
from app.maintenance.maintenance_manager import maintenance_init_redis_conn
from app.factory import create_app
from app.models import ClusterInfo, db, User, UserContainer


@pytest.fixture
def _retrieve_user():
    """Instruct the user fixture to retrieve the saved user_id.

    Don't use this fixture because it is a temporary hack.

    This fixture should be used very rarely in conjunction with the _save_user
    test fixture to instruct the user fixture to retrieve from the global
    pytest object the user_id of the saved user rather than creating a new
    user.
    """

    pass


@pytest.fixture
def _save_user():
    """Instruct the user fixture to save the test user instead of deleting it.

    Don't use this fixture because it is a temporary hack.

    This fixture should be used very rarely in conjunction with the
    _retrieve_user test fixture to instruct the user fixture to save the test
    user's user_id to the global pytest object instead of deleting the user
    during teardown.
    """

    pass


@pytest.fixture
def admin(app, authorized, monkeypatch):
    """Bypass the admin_required decorator.

    The way the admin_required decorator works is it compares the identity stored in the JWT token
    sent along with the request to the admin username. Since we patch the get_jwt_identity function
    so it always returns is the identity of our test user, we can easily bypass the admin_required
    decorator by patching the admin username to be the same as the one that the patched
    get_jwt_identity function returns.

    Returns:
        An instance of the User model representing the authorized admin user.
    """

    monkeypatch.setitem(app.config, "DASHBOARD_USERNAME", authorized.user_id)

    return authorized


@pytest.fixture(scope="session")
def app():
    """Flask application test fixture required by pytest-flask.

    https://pytest-flask.readthedocs.io/en/latest/tutorial.html#step-2-configure.

    For now, This test fixture must have session scope because the session-
    scoped celery_parameters test fixture depends on it and session-scoped
    fixtures cannot depend on other fixtures with more granular scopes. If you
    don't believe me, just try changing this fixture's scope to be more
    granular than session scope. Admittedly, having only a single instance of
    the Flask application for each test session might be less than ideal. It
    would be good to explore alternative solutions that would allow this
    fixture to have function scope.

    Returns:
        An instance of the Flask application for testing.
    """

    _app = create_app(testing=True)
    from app.helpers.utils.general.logs import fractal_logger

    fractal_logger.info(f"REDIS URL: {_app.config['REDIS_URL']}")
    maintenance_init_redis_conn(_app.config["REDIS_URL"])

    return _app


@pytest.fixture
def authorized(client, user, monkeypatch):
    """Bypass authorization decorators.

    Inject the JWT bearer token of an authorized user into the HTTP Authorization header that is
    sent along with every request made by the Flask test client.

    Returns:
        An instance of the User model representing the authorized user.
    """

    access_token = create_access_token(identity=user.user_id)

    # environ_base contains base data that is used to construct every request that the client
    # sends. Here, we are injecting a value into the field that contains the base HTTP
    # Authorization header data.
    monkeypatch.setitem(client.environ_base, "HTTP_AUTHORIZATION", f"Bearer {access_token}")

    return user


@pytest.fixture(scope="session")
def celery_config():
    """Set celery configuration variables.

    https://docs.celeryproject.org/en/latest/userguide/testing.html#session-scope.

    Returns:
        A dictionary whose keys are celery configuration variables.
    """

    return CELERY_CONFIG


@pytest.fixture(scope="session")
def celery_parameters(app):
    """Generate celery keyword arguments.

    https://docs.celeryproject.org/en/latest/userguide/testing.html#session-scope.

    Returns:
        A dictionary whose keys are keyword arguments that will be passed to the Celery
        constructor.
    """

    return celery_params(app)


@pytest.fixture(scope="session")
def celery_enable_logging():
    return True


@pytest.fixture
def cluster():
    """Add a row to the cluster_info of the database for testing.

    Returns:
        An instance of the ClusterInfo model.
    """

    c = ClusterInfo(cluster=f"test-cluster-{uuid.uuid4()}")

    db.session.add(c)
    db.session.commit()

    yield c

    db.session.delete(c)
    db.session.commit()


@pytest.fixture
def container(cluster, user):
    """Add a row to the user_containers table for testing.

    Returns:
        A context manager that populates the user_containers table with a test
        row whose state column is set to initial_state.
    """

    @contextmanager
    def _container(initial_state="CREATING"):
        """Create a dummy container for testing.

        Arguments:
            initial_state: The initial value with which the new row's state
                column should be populated.

        Yields:
            An instance of the UserContainer model.
        """

        c = UserContainer(
            container_id=f"{os.urandom(16).hex()}",
            ip=f"{randbits(7)}.{randbits(7)}.{randbits(7)}.{randbits(7)}",
            location="us-east-1",
            os="Linux",
            state=initial_state,
            user_id=user.user_id,
            port_32262=randbits(16),
            port_32263=randbits(16),
            port_32273=randbits(16),
            cluster=cluster.cluster,
            secret_key=os.urandom(16).hex(),
        )

        db.session.add(c)
        db.session.commit()

        yield c

        db.session.delete(c)
        db.session.commit()

    return _container


@pytest.fixture
def user(request):
    """Create a test user.

    Returns:
        An instance of the User model.
    """

    if "_retrieve_user" not in request.fixturenames:
        u = User(
            user_id=f"test-user+{uuid.uuid4()}@fractal.co",
            password="",
            created_timestamp=1000000000,
        )

        db.session.add(u)
        db.session.commit()
    else:
        u = User.query.get(pytest._user_id)

    yield u

    if "_save_user" not in request.fixturenames:
        db.session.delete(u)
        db.session.commit()
    else:
        pytest._user_id = u.user_id


@pytest.fixture
def make_user():
    """Create a new user for testing purposes.

    Args:
        stripe_customer_id: Optional. The test user's Stripe customer ID.
        created_timestamp: Optional. An arbitrary time at which the new user was created. Should be
            a timezone-aware time stamp. Defaults to the current time.

    Returns:
        An instance of the User model.
    """

    # Maintain a list of all of the users that have been created with this factory fixture so they
    # can be deleted during this fixture's teardown phase.
    users = []

    def _user(stripe_customer_id=None, created_timestamp=datetime.now(timezone.utc)):
        user = User(
            user_id=f"test-user+{uuid.uuid4()}@fractal.co",
            password="",
            created_timestamp=created_timestamp,
            stripe_customer_id=stripe_customer_id,
        )

        db.session.add(user)
        db.session.commit()
        users.append(user)

        return user

    yield _user

    # Clean up all of the test users that were created.
    for user in users:
        db.session.delete(user)

    db.session.commit()


@pytest.fixture
def make_authorized_user(client, make_user, monkeypatch):
    """Create a new user for testing purposes and authorize all future test requests as that user.

    Args:
        stripe_customer_id: Optional. The test user's Stripe customer ID.
        created_timestamp: Optional. An arbitrary time at which the new user was created. Should be
            a timezone-aware time stamp. Defaults to the current time.

    Returns:
        An instance of the User model representing the authorized user.
    """

    def _authorized_user(stripe_customer_id=None, created_timestamp=datetime.now(timezone.utc)):
        user = make_user(
            stripe_customer_id=stripe_customer_id,
            created_timestamp=created_timestamp,
        )
        access_token = create_access_token(identity=user.user_id)

        monkeypatch.setitem(client.environ_base, "HTTP_AUTHORIZATION", f"Bearer {access_token}")

        return user

    return _authorized_user
