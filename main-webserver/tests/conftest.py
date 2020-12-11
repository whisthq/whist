import os
import uuid

from contextlib import contextmanager
from random import getrandbits as randbits

import pytest

from celery.app.task import Task
from flask_jwt_extended import get_jwt_identity, verify_jwt_in_request

from app.factory import create_app
from app.models import ClusterInfo, db, User, UserContainer

from .patches import do_nothing


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

    return _app


@pytest.fixture
def authorized(user, monkeypatch):
    """Disable authentication decorators.

    By reading through the source code of the flask_jwt_extended package, I
    figured out that get_jwt_identity and verify_jwt_in_request were the
    functions responsible for most of the heavy lifting required to implement
    the functionality that flask_jwt_extended provides.

    Returns:
        An instance of the User model representing the authorized user.
    """

    _get_jwt_identity = eval(f"""lambda: "{user.user_id}" """)

    monkeypatch.setattr(get_jwt_identity, "__code__", _get_jwt_identity.__code__)
    monkeypatch.setattr(verify_jwt_in_request, "__code__", do_nothing.__code__)

    return user


@pytest.fixture(scope="session")
def celery_config():
    """Configure the Celery application for testing.

    https://docs.celeryproject.org/en/latest/userguide/testing.html#session-scope.
    """

    redis_url = os.environ.get("REDIS_URL", "redis://")

    return {
        "broker_url": redis_url,
        "result_backend": redis_url,
    }


@pytest.fixture(scope="session")
def celery_parameters(app):
    """Continue configuring the Celery application for testing.

    https://docs.celeryproject.org/en/latest/userguide/testing.html#session-scope.
    """

    class ContextTask(Task):
        def __call__(self, *args, **kwargs):
            with app.app_context():
                return self.run(*args, **kwargs)

    return {
        "task_cls": ContextTask,
    }


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
            container_id=f"test-container-{uuid.uuid4()}",
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


@pytest.fixture(autouse=True)
def mock_aws(monkeypatch, pytestconfig):
    """Patch environment variables to override legitimate AWS credentials.

    Prevents accidental modification of actual AWS resources.

    The mock_aws key in the pytest configuration is set to True by default.
    Since this fixture is an autouse fixture, the AWS credentials used in tests
    are set to garbage values (e.g. "mock") by default. If the proper fixtures,
    which can be found in tests/aws/services.py, are not applied to test
    functions, those tests functions will not work because they will try to use
    garbage credentials to access the real AWS API. However, once the correct
    fixtures are applied to the test functions, it will be possible to toggle
    between using the real AWS API and the mock AWS API with the --mock-aws/
    --no-mock-aws command line flags.
    """

    if pytestconfig.getoption("mock_aws"):
        monkeypatch.setenv("AWS_ACCESS_KEY_ID", "mock")
        monkeypatch.setenv("AWS_SECRET_ACCESS_KEY", "mock")
        monkeypatch.setenv("AWS_SECURITY_TOKEN", "mock")
        monkeypatch.setenv("AWS_SESSION_TOKEN", "mock")


def pytest_addoption(parser, pluginmanager):
    """Add command line options to explicitly enable or disable moto.

    https://docs.pytest.org/en/stable/reference.html#pytest.hookspec.pytest_addoption.
    """

    parser.addoption("--mock-aws", dest="mock_aws", default=True, action="store_true")
    parser.addoption("--no-mock-aws", dest="mock_aws", action="store_false")


@pytest.fixture
def user(request):
    """Create a test user.

    Returns:
        An instance of the User model.
    """

    if "_retrieve_user" not in request.fixturenames:
        u = User(user_id=f"test-user+{uuid.uuid4()}@tryfractal.com", password="")

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
