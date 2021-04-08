import os
import uuid

from contextlib import contextmanager
from random import getrandbits as randbits
import platform
import subprocess
import signal

import pytest

from flask_jwt_extended import create_access_token

from app.celery_utils import CELERY_CONFIG, celery_params
from app.maintenance.maintenance_manager import maintenance_init_redis_conn
from app.factory import create_app
from app.models import ClusterInfo, db, User, UserContainer
import app.constants.env_names as env_names
from app.flask_handlers import set_web_requests_status
from app.signals import WebSignalHandler
from app.helpers.utils.general.logs import fractal_logger
from app.helpers.utils.general.limiter import limiter
from app.celery_utils import make_celery


@pytest.fixture
def _retrieve_user():
    """Instruct the user fixture to retrieve the saved user_id.

    Don't use this fixture because it is a temporary hack.

    This fixture should be used very rarely in conjunction with the _save_user
    test fixture to instruct the user fixture to retrieve from the global
    pytest object the user_id of the saved user rather than creating a new
    user.
    """


@pytest.fixture
def _save_user():
    """Instruct the user fixture to save the test user instead of deleting it.

    Don't use this fixture because it is a temporary hack.

    This fixture should be used very rarely in conjunction with the
    _retrieve_user test fixture to instruct the user fixture to save the test
    user's user_id to the global pytest object instead of deleting the user
    during teardown.
    """


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
    # TODO: this entire function generally the same as entry_web.py. Can we combine?
    _app = create_app(testing=True)

    # enable web requests
    if not set_web_requests_status(True):
        fractal_logger.fatal("Could not enable web requests at startup. Failing out.")
        os.sys.exit(1)

    # enable the web signal handler. This should work on OSX and Linux.
    if "windows" in platform.platform().lower():
        fractal_logger.warning(
            "signal handler is not supported on windows. skipping enabling them."
        )
    else:
        WebSignalHandler()

    # initialize redis connection for maintenance package
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

    c = ClusterInfo(cluster=f"test-cluster-{uuid.uuid4()}", location="us-east-1")

    db.session.add(c)
    db.session.commit()

    yield c

    db.session.delete(c)
    db.session.commit()


@pytest.fixture
def container(cluster, user, task_def_env):
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
            task_definition=f"fractal-{task_def_env}-browsers-chrome",
            task_version=None,
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

    yield _container


@pytest.fixture
def bulk_cluster():
    """Add 1+ rows to the clusters table for testing.

    Returns:
        A function that populates the clusterInfo table with a test
        row whose columns are set as arguments to the function.
    """
    clusters = []

    def _cluster(cluster_name=None, location=None, **kwargs):
        """Create a dummy cluster for testing.

        Arguments:
            cluster_name (Optional[str]): what to call the cluster
                    defaults to random name
            location (Optional[str]): what region to put the cluster in
                    defaults to us-east-1

        Yields:
            An instance of the ClusterInfo model.
        """
        c = ClusterInfo(
            cluster=cluster_name if cluster_name is not None else f"cluster-{os.urandom(16).hex()}",
            location=location if location is not None else "us-east-1",
            status="CREATED",
            **kwargs,
        )

        db.session.add(c)
        db.session.commit()
        clusters.append(c)

        return c

    yield _cluster

    for cluster in clusters:
        db.session.delete(cluster)

    db.session.commit()


@pytest.fixture
def bulk_container(bulk_cluster, make_user, task_def_env):
    """Add 1+ rows to the user_containers table for testing.

    In the absence of this fixture's explicit dependence on the make_user fixture (i.e. when
    make_user is not present in this fixture's argument list), the make_user teardown code is run
    before this fixture's teardown code, causing all containers assigned to users created with
    make_user() to be CASCADE deleted. In order to prevent the user who owns a container created by
    bulk_container() from being CASCADE deleted before this fixture's teardown code is run,
    make_user has been added to this fixture's argument list.

    Returns:
        A function that populates the user_containers table with a test
        row whose state column is set to initial_state.
    """
    containers = []

    def _container(*, assigned_to=None, location="us-east-1", container_id=None):
        """Create a dummy container for testing.

        Arguments:
            assigned_to: A string representing the user ID of the user to whom the container should
                be assigned. Dummy prewarmed containers may be created with
                bulk_container(assigned_to=None)

            location:  which region to create the container in
            container_id:  the specific name we want the container to have
                           useful for testing which container object is retrieved
        Yields:
            An instance of the UserContainer model.
        """
        cluster = bulk_cluster(location=location)
        c = UserContainer(
            container_id=container_id if container_id is not None else f"{os.urandom(16).hex()}",
            ip=f"{randbits(7)}.{randbits(7)}.{randbits(7)}.{randbits(7)}",
            location=location,
            task_definition=f"fractal-{task_def_env}-browsers-chrome",
            task_version=None,
            state="CREATING",
            user_id=assigned_to,
            port_32262=randbits(16),
            port_32263=randbits(16),
            port_32273=randbits(16),
            cluster=cluster.cluster,
            secret_key=os.urandom(16).hex(),
        )

        db.session.add(c)
        db.session.commit()
        containers.append(c)

        return c

    yield _container

    for container in containers:
        db.session.delete(container)

    db.session.commit()


@pytest.fixture(scope="session")
def task_def_env(app):
    """Determine what the environment portion of the task_definition IDs should be set to.

    Tests for production code are run against "prod" AWS resources. Tests for staging code are run
    against "staging" AWS resources. Tests for "dev" code and local code are run against "dev" AWS
    resources.

    Returns:
        Either "dev", "staging", or "prod".
    """

    task_def_envs = {
        env_names.PRODUCTION: "prod",
        env_names.STAGING: "staging",
        env_names.DEVELOPMENT: "dev",
        env_names.TESTING: "dev",
        env_names.LOCAL: "dev",
    }

    return task_def_envs[app.config["ENVIRONMENT"]]


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
            encrypted_config_token="",
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
        domain (Optional[str]): Which domain the user email address should be in.
        kwargs: Optional. Additional keyword arguments that will be forwarded directly to the User
            constructor. If provided, the user_id, password, and encrypted_config_token keyword
            arguments will be ignored.

    Returns:
        An instance of the User model.
    """

    # Maintain a list of all of the users that have been created with this factory fixture so they
    # can be deleted during this fixture's teardown phase.
    users = []

    def _user(stripe_customer_id=None, domain="fractal.co", **kwargs):
        kwargs["user_id"] = f"test-user-{uuid.uuid4()}@{domain}"
        kwargs["password"] = kwargs["encrypted_config_token"] = ""
        user = User(stripe_customer_id=stripe_customer_id, **kwargs)

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
        domain (Optional[str]): Which domain the user email address should be in.
        kwargs: Optional. Additional keyword arguments that will be forwarded directly to the User
            constructor. The user_id and password keyword arguments will be ignored if they are
            provided.

    Returns:
        An instance of the User model representing the authorized user.
    """

    def _authorized_user(stripe_customer_id=None, domain="fractal.co", **kwargs):
        user = make_user(stripe_customer_id=stripe_customer_id, domain=domain, **kwargs)
        access_token = create_access_token(identity=user.user_id)

        monkeypatch.setitem(client.environ_base, "HTTP_AUTHORIZATION", f"Bearer {access_token}")

        return user

    return _authorized_user


@pytest.fixture
def fractal_celery_app(app):
    """
    Initialize celery like we do in entry_web.py. This is different than the built-in
    celery_app fixture and works hand-in-hand with fractal_celery_proc.
    """
    celery_app = make_celery(app)
    celery_app.set_default()
    yield celery_app


@pytest.fixture
def fractal_celery_proc(app):
    """
    Run a celery worker like we do in Procfile/stem-cell.sh
    No monkeypatched code will apply to this worker.
    """
    # this gets the webserver root no matter where this file is called from.
    webserver_root = os.path.join(os.getcwd(), os.path.dirname(__file__), "..")

    # these are used by supervisord
    os.environ["NUM_WORKERS"] = "2"
    os.environ["WORKER_CONCURRENCY"] = "10"

    # stdout is shared but the process is run separately. Signals sent to the current process
    # are also sent to this process. This is the exact command run by Procfile/stem-cell.sh
    # for celery workers.
    proc = subprocess.Popen(
        ["supervisord", "-c", "supervisor.conf"],
        shell=False,
    )

    fractal_logger.info(f"Started celery process with pid {proc.pid}")

    # this is the pid of the shell that launches celery. See:
    # https://stackoverflow.com/questions/31039972/python-subprocess-popen-pid-return-the-pid-of-the-parent-script
    yield proc.pid

    # we need to kill the process group because a new shell was launched which then launched celery
    fractal_logger.info(f"Killing celery process with pid {proc.pid}")
    try:
        os.kill(proc.pid, signal.SIGKILL)
    except (PermissionError, ProcessLookupError):
        # some tests kill this process themselves; in this case us trying to kill an already killed
        # process results in a PermissionError. We cleanly catch that specific error.
        pass


@pytest.fixture(autouse=True)
def reset_limiter():
    """
    Reset the rate limiter after every test.
    """
    limiter.reset()
