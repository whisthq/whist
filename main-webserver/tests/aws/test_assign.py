"""Tests for the /mandelbox/assign endpoint."""

from http import HTTPStatus

from types import SimpleNamespace

import pytest


from app.models import SupportedAppImages


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
    instance = bulk_instance(instance_name="mock_instance_id", ip="123.456.789")

    def patched_find(*args, **kwargs):
        return instance.instance_name

    monkeypatch.setattr(
        "app.blueprints.aws.aws_container_blueprint.find_instance",
        patched_find,
    )

    args = {"region": "us-east-1", "username": "neil@fractal.co", "dpi": 96}
    response = client.post("/mandelbox/assign", json=args)

    assert response.json["IP"] == instance.ip


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
@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
def test_payment(admin, client, make_user, monkeypatch, status_code, subscribed):
    user = make_user()
    response = client.post(
        "/mandelbox/assign",
        json={
            "username": user,
            "app": "Google Chrome",
            "region": "us-east-1",
        },
    )

    assert response.status_code == status_code
