"""Tests for the /mandelbox/assign endpoint."""
from random import randint
from typing import Any, Callable

from http import HTTPStatus
from flask import Flask
import pytest


from app.constants import CLIENT_COMMIT_HASH_DEV_OVERRIDE
from app.constants.env_names import DEVELOPMENT, PRODUCTION
from app.models import Instance
from tests.client import WhistAPITestClient
from tests.constants import CLIENT_COMMIT_HASH_FOR_TESTING
from tests.helpers.utils import get_allowed_region_names


@pytest.mark.usefixtures("authorized")
def test_bad_app(client: WhistAPITestClient) -> None:
    response = client.post("/mandelbox/assign", json=dict(app="Bad App"))

    assert response.status_code == HTTPStatus.BAD_REQUEST


@pytest.mark.usefixtures("authorized")
def test_no_app(client: WhistAPITestClient) -> None:
    response = client.post("/mandelbox/assign", json={})

    assert response.status_code == HTTPStatus.BAD_REQUEST


@pytest.mark.usefixtures("authorized")
def test_no_region(client: WhistAPITestClient) -> None:
    response = client.post("/mandelbox/assign", json=dict(app="VSCode"))

    assert response.status_code == HTTPStatus.BAD_REQUEST


@pytest.mark.usefixtures("authorized")
def test_assign(
    client: WhistAPITestClient,
    bulk_instance: Callable[..., Instance],
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    instance = bulk_instance(instance_name="mock_instance_name")

    def patched_find(*args: Any, **kwargs: Any) -> Any:  # pylint: disable=unused-argument
        return instance.id

    monkeypatch.setattr(
        "app.api.mandelbox.find_instance",
        patched_find,
    )

    args = {
        "regions": get_allowed_region_names(),
        "client_commit_hash": CLIENT_COMMIT_HASH_FOR_TESTING,
        "session_id": randint(1600000000, 9999999999),
        "user_email": "test-user@whist.com",
    }
    response = client.post("/mandelbox/assign", json=args)

    assert response.json["ip"] == instance.ip_addr
    assert response.json["mandelbox_id"] != "" and response.json["mandelbox_id"] != "None"


@pytest.mark.skip(reason="We currently ignore user activity.")
@pytest.mark.usefixtures("authorized")
def test_assign_active(
    client: WhistAPITestClient,
    bulk_instance: Callable[..., Instance],
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """
    Ensures we 503 a user with active mandelboxes
    """
    bulk_instance(instance_name="mock_instance_name")

    def patched_active(*args: Any, **kwargs: Any) -> bool:  # pylint: disable=unused-argument
        return True

    monkeypatch.setattr(
        "app.api.mandelbox.is_user_active",
        patched_active,
    )

    args = {
        "regions": get_allowed_region_names(),
        "client_commit_hash": CLIENT_COMMIT_HASH_FOR_TESTING,
        "session_id": randint(1600000000, 9999999999),
    }
    response = client.post("/mandelbox/assign", json=args)

    assert response.status_code == HTTPStatus.SERVICE_UNAVAILABLE


@pytest.mark.usefixtures("authorized")
def test_client_commit_hash_local_dev_override_fail(
    app: Flask,  # pylint: disable=unused-argument
    client: WhistAPITestClient,
    bulk_instance: Callable[..., Instance],
    override_environment: Callable[[str], None],
) -> None:
    """
    Ensure that in production environment, passing the pre-shared client commit hash for dev
    enviroment returns a status code of RESOURCE_UNAVAILABLE
    """

    override_environment(PRODUCTION)
    region_names = get_allowed_region_names()
    bulk_instance(instance_name="mock_instance_name", location=region_names[0])

    args = {
        "regions": region_names,
        "client_commit_hash": CLIENT_COMMIT_HASH_DEV_OVERRIDE,
        "session_id": randint(1600000000, 9999999999),
        "user_email": "test-user@whist.com",
    }
    response = client.post("/mandelbox/assign", json=args)

    assert response.status_code == HTTPStatus.SERVICE_UNAVAILABLE


@pytest.mark.usefixtures("authorized")
def test_client_commit_hash_local_dev_override_success(
    app: Flask,  # pylint: disable=unused-argument
    client: WhistAPITestClient,
    bulk_instance: Callable[..., Instance],
    override_environment: Callable[[str], None],
) -> None:
    """
    Ensure that in development environment, passing the pre-shared client commit hash for dev
    enviroment returns a status code of ACCEPTED
    """

    override_environment(DEVELOPMENT)
    region_names = get_allowed_region_names()
    bulk_instance(instance_name="mock_instance_name", location=region_names[0])

    args = {
        "regions": region_names,
        "client_commit_hash": CLIENT_COMMIT_HASH_DEV_OVERRIDE,
        "session_id": randint(1600000000, 9999999999),
        "user_email": "test-user@whist.com",
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
def test_payment(
    admin: bool,  # pylint: disable=unused-argument
    client: WhistAPITestClient,
    make_user: Callable[..., str],
    monkeypatch: pytest.MonkeyPatch,  # pylint: disable=unused-argument
    status_code: HTTPStatus,
    subscribed: bool,  # pylint: disable=unused-argument
) -> None:
    user = make_user()  # pylint: disable=unused-variable
    response = client.post(
        "/mandelbox/assign",
        json={
            "app": "Google Chrome",
            "region": get_allowed_region_names(),
            "session_id": randint(1600000000, 9999999999),
            "user_email": "test-user@whist.com",
        },
    )

    assert response.status_code == status_code
