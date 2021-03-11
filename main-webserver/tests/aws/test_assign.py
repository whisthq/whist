"""Tests for the /container/assign endpoint."""

import datetime

from datetime import timedelta, datetime as dt
from http import HTTPStatus

import pytest
import time


from app.celery.aws_ecs_creation import assign_container
from app.constants.time import SECONDS_IN_MINUTE, MINUTES_IN_HOUR, HOURS_IN_DAY
from app.helpers.utils.payment.stripe_client import StripeClient
from app.serializers.public import UserSchema

from ..patches import function, Object


def test_bad_app(client, authorized, monkeypatch):
    response = client.post(
        "/container/assign", json=dict(username=authorized.user_id, app="Bad App")
    )
    assert response.status_code == HTTPStatus.BAD_REQUEST


def test_no_username(client, authorized):
    response = client.post("/container/assign", json=dict(app="VSCode"))
    assert response.status_code == HTTPStatus.UNAUTHORIZED


def test_no_app(client, authorized, monkeypatch):
    response = client.post("/container/assign", json=dict(username=authorized.user_id))

    assert response.status_code == HTTPStatus.BAD_REQUEST


def test_no_region(client, authorized, monkeypatch):
    response = client.post(
        "/container/assign", json=dict(username=authorized.user_id, app="VSCode")
    )

    assert response.status_code == HTTPStatus.BAD_REQUEST


@pytest.mark.parametrize(
    "email_domain, subscribed, status_code",
    (
        ("fractal.co", True, HTTPStatus.ACCEPTED),
        ("fractal.co", False, HTTPStatus.ACCEPTED),
        ("gmail.com", True, HTTPStatus.ACCEPTED),
        ("gmail.com", False, HTTPStatus.PAYMENT_REQUIRED),
    ),
)
@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
def test_payment(client, email_domain, make_authorized_user, monkeypatch, status_code, subscribed):
    task = Object()
    user = make_authorized_user(domain=email_domain, subscribed=subscribed)

    monkeypatch.setattr(assign_container, "run", function())

    response = client.post(
        "/container/assign",
        json={
            "username": user.user_id,
            "app": "Google Chrome",
            "region": "us-east-1",
        },
    )

    assert response.status_code == status_code
