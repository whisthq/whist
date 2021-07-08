import os
import uuid
import time

from random import randint
import platform

import pytest

from flask_jwt_extended import create_access_token
from flask_jwt_extended.default_callbacks import default_decode_key_callback

from app.factory import create_app
from app.models import MandelboxInfo, db, InstanceInfo, RegionToAmi
from app.flask_handlers import set_web_requests_status
from app.signals import WebSignalHandler
from app.helpers.utils.general.logs import fractal_logger
from app.helpers.utils.general.limiter import limiter
from tests.client import FractalAPITestClient
from tests.constants import CLIENT_COMMIT_HASH_FOR_TESTING
from tests.helpers.utils import get_random_regions, get_random_region_name, get_random_region_names


@pytest.fixture(scope="session")
def app():
    """Flask application test fixture required by pytest-flask.

    https://pytest-flask.readthedocs.io/en/latest/tutorial.html#step-2-configure.

    TODO: Check if anymore cleanup here is needed since we removed celery.

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
    _app.test_client_class = FractalAPITestClient

    # Reconfigure Flask-JWT-Extended so it can validate test JWTs
    _app.extensions["flask-jwt-extended"].decode_key_loader(default_decode_key_callback)

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

    return _app


@pytest.fixture
def authorized(client, user, monkeypatch):
    """Bypass authorization decorators.

    Inject the JWT bearer token of an authorized user into the HTTP Authorization header that is
    sent along with every request made by the Flask test client.

    Returns:
        A string representing the authorized user's identity.
    """
    access_token = create_access_token(identity=user, additional_claims={"scope": "admin"})

    # environ_base contains base data that is used to construct every request that the client
    # sends. Here, we are injecting a value into the field that contains the base HTTP
    # Authorization header data.
    monkeypatch.setitem(client.environ_base, "HTTP_AUTHORIZATION", f"Bearer {access_token}")

    return user


@pytest.fixture
def bulk_instance():
    """Add 1+ rows to the instance_info table for testing.

    Returns:
        A function that populates the instanceInfo table with a test
        row whose columns are set as arguments to the function.
    """
    instances = []

    def _instance(
        associated_mandelboxes=0,
        instance_name=None,
        location=None,
        mandelbox_capacity=None,
        **kwargs,
    ):
        """Create a dummy instance for testing.

        Arguments:
            associated_mandelboxes (int): How many mandelboxes should be made running
                on this instance
            instance_name (Optional[str]): what to call the instance
                    defaults to random name
            location (Optional[str]): what region to put the instance in
                    defaults to us-east-1
            mandelbox_capacity (Optional[int]): how many mandelboxes can the instance hold?
                defaults to 10

        Yields:
            An instance of the InstanceInfo model.
        """
        inst_name = (
            instance_name if instance_name is not None else f"instance-{os.urandom(16).hex()}"
        )
        new_instance = InstanceInfo(
            instance_name=inst_name,
            cloud_provider_id=f"aws-{inst_name}",
            location=location if location is not None else "us-east-1",
            creation_time_utc_unix_ms=int(time.time() * 1000),
            mandelbox_capacity=mandelbox_capacity if mandelbox_capacity is not None else 10,
            ip=kwargs.get("ip", "123.456.789"),
            aws_ami_id=kwargs.get("aws_ami_id", "test"),
            aws_instance_type=kwargs.get("aws_instance_type", "test_type"),
            last_updated_utc_unix_ms=kwargs.get("last_updated_utc_unix_ms", 10),
            status=kwargs.get("status", "ACTIVE"),
            commit_hash=CLIENT_COMMIT_HASH_FOR_TESTING,
        )

        db.session.add(new_instance)
        db.session.commit()
        for _ in range(associated_mandelboxes):
            new_mandelbox = MandelboxInfo(
                mandelbox_id=str(randint(0, 10000000)),
                instance_name=new_instance.instance_name,
                user_id=kwargs.get("user_for_mandelboxes", "test-user"),
                status="Running",
                creation_time_utc_unix_ms=int(time.time() * 1000),
            )
            db.session.add(new_mandelbox)
            db.session.commit()

        instances.append(new_instance)

        return new_instance

    yield _instance

    for instance in instances:
        db.session.delete(instance)
    # We only need to delete the instances for cleanup. mandelboxes have a foreign key
    # relationship with instances on instance_name column and due to cascade on delete/update
    # the mandelboxes will be deleted on deletion of corresponding instances.

    db.session.commit()


@pytest.fixture
def region_to_ami_map(app):
    """
    Returns a dict of active <Region:AMI> pairs.
    """
    all_regions = RegionToAmi.query.all()
    region_map = {region.region_name: region.ami_id for region in all_regions if region.ami_active}
    return region_map

@pytest.fixture
def region_ami_pair():
    """
    Returns a randomly picked region and corresponding am
    """
    region_ami_pair = get_random_regions(1)
    if region_ami_pair is not None:
        return region_ami_pair[0].region_name, region_ami_pair[0].ami_id 

@pytest.fixture
def region_names():
    """
    Returns a randomly picked region
    """
    return get_random_region_names()

@pytest.fixture
def region_name():
    """
    Returns a randomly picked region
    """
    region_name = get_random_region_name()
    return region_name


@pytest.fixture
def override_environment(app):
    """
    Override the environment temporarily to test environment specific behaviour.

    Example:
    POST `/mandelbox/assign` requires a client commit hash to be sent for matching the client application
    to a compatible instance. However, in dev environment, for `/mandelbox/assign` call we accept a pre-shared
    static client_commit_hash along with the entry in the database to figure out the latest active AMI.
    """
    environment_ = None

    def _environment(_value):
        nonlocal environment_
        environment_ = app.config["ENVIRONMENT"]
        app.config["ENVIRONMENT"] = _value

    yield _environment
    app.config["ENVIRONMENT"] = environment_


@pytest.fixture
def make_user():
    """Create a new user for testing purposes.

    See tests.conftest.user.
    """

    return user


@pytest.fixture
def make_authorized_user(client, make_user, monkeypatch):
    """Create a new user for testing purposes and authorize all future test requests as that user.

    See tests.conftest.user.
    """

    def _authorized_user(**kwargs):
        username = make_user(**kwargs)
        access_token = create_access_token(identity=username, additional_claims={"scope": "admin"})

        monkeypatch.setitem(client.environ_base, "HTTP_AUTHORIZATION", f"Bearer {access_token}")

        return username

    return _authorized_user


@pytest.fixture(autouse=True)
def reset_limiter():
    """
    Reset the rate limiter after every test.
    """
    limiter.reset()


def user(*, domain="fractal.co", user_id=None):
    """Generate a fake email address for a test user.

    Args:
        domain: Optional. A string representing a domain name. It will be the domain with which the
            user's email address is associated.

    Returns:
        A string representing the user's identity.
    """

    return user_id if user_id is not None else f"test-user+{uuid.uuid4()}@fractal.co"


user_fixture = pytest.fixture(name="user")(user)
