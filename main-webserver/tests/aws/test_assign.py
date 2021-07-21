"""Tests for the /mandelbox/assign endpoint."""

from http import HTTPStatus
import pytest


from app.models import SupportedAppImages
from app.constants import CLIENT_COMMIT_HASH_DEV_OVERRIDE
from app.constants.env_names import DEVELOPMENT, PRODUCTION
from tests.constants import CLIENT_COMMIT_HASH_FOR_TESTING
from tests.helpers.utils import get_random_region_name


@pytest.mark.usefixtures("authorized")
def test_bad_app(client):
    response = client.post("/mandelbox/assign", json=dict(app="Bad App"))

    assert response.status_code == HTTPStatus.BAD_REQUEST


@pytest.mark.usefixtures("authorized")
def test_no_app(client):
    response = client.post("/mandelbox/assign", json=dict())

    assert response.status_code == HTTPStatus.BAD_REQUEST


@pytest.mark.usefixtures("authorized")
def test_no_region(client):
    response = client.post("/mandelbox/assign", json=dict(app="VSCode"))

    assert response.status_code == HTTPStatus.BAD_REQUEST


@pytest.mark.usefixtures("authorized")
def test_assign(client, bulk_instance, monkeypatch):
    instance = bulk_instance(instance_name="mock_instance_name", ip="123.456.789")

    def patched_find(*args, **kwargs):
        return instance.instance_name

    monkeypatch.setattr(
        "app.blueprints.aws.aws_mandelbox_blueprint.find_instance",
        patched_find,
    )

    args = {
        "region": get_random_region_name(),
        "username": "test@fractal.co",
        "dpi": 96,
        "client_commit_hash": CLIENT_COMMIT_HASH_FOR_TESTING,
    }
    response = client.post("/mandelbox/assign", json=args)

    assert response.json["ip"] == instance.ip
    assert response.json["mandelbox_id"] != "" and response.json["mandelbox_id"] != "None"


@pytest.mark.skip(reason="We currently ignore user activity.")
@pytest.mark.usefixtures("authorized")
def test_assign_active(client, bulk_instance, monkeypatch):
    """
    Ensures we 503 a user with active mandelboxes
    """
    bulk_instance(instance_name="mock_instance_name", ip="123.456.789")

    def patched_active(*args, **kwargs):
        return True

    monkeypatch.setattr(
        "app.blueprints.aws.aws_mandelbox_blueprint.is_user_active",
        patched_active,
    )

    args = {
        "region": get_random_region_name(),
        "username": "test@fractal.co",
        "dpi": 96,
        "client_commit_hash": CLIENT_COMMIT_HASH_FOR_TESTING,
    }
    response = client.post("/mandelbox/assign", json=args)

    assert response.status_code == HTTPStatus.SERVICE_UNAVAILABLE


@pytest.mark.usefixtures("authorized")
def test_client_commit_hash_local_dev_override_fail(
    app, client, bulk_instance, override_environment
):
    """
    Ensure that in production environment, passing the pre-shared client commit hash for dev enviroment
    returns a status code of RESOURCE_UNAVAILABLE
    """

    override_environment(PRODUCTION)
    region_name = get_random_region_name()
    bulk_instance(instance_name="mock_instance_name", ip="123.456.789", location=region_name)

    args = {
        "region": region_name,
        "username": "test@fractal.co",
        "dpi": 96,
        "client_commit_hash": CLIENT_COMMIT_HASH_DEV_OVERRIDE,
    }
    response = client.post("/mandelbox/assign", json=args)

    assert response.status_code == HTTPStatus.SERVICE_UNAVAILABLE


@pytest.mark.usefixtures("authorized")
def test_client_commit_hash_local_dev_override_success(
    app, client, bulk_instance, override_environment
):
    """
    Ensure that in development environment, passing the pre-shared client commit hash for dev enviroment
    returns a status code of ACCEPTED
    """

    override_environment(DEVELOPMENT)
    region_name = get_random_region_name()
    bulk_instance(instance_name="mock_instance_name", ip="123.456.789", location=region_name)

    args = {
        "region": region_name,
        "username": "test@fractal.co",
        "dpi": 96,
        "client_commit_hash": CLIENT_COMMIT_HASH_DEV_OVERRIDE,
    }
    response = client.post("/mandelbox/assign", json=args)

    assert response.status_code == HTTPStatus.ACCEPTED


@pytest.mark.skip(reason="The @payment_required() decorator is not implemented yet.")
@pytest.mark.parametrize(
    "admin, subscribed, status_code",
    (
        (True, True, HTTPStatus.ACCEPTED),
        (True, False, HTTPStatus.ACCEPTED),
        (False, True, HTTPStatus.ACCEPTED),
        (False, False, HTTPStatus.PAYMENT_REQUIRED),
    ),
)
def test_payment(admin, client, make_user, monkeypatch, status_code, subscribed):
    user = make_user()
    response = client.post(
        "/mandelbox/assign",
        json={
            "username": user,
            "app": "Google Chrome",
            "region": get_random_region_name(),
        },
    )

    assert response.status_code == status_code
