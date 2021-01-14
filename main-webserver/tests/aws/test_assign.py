"""Tests for the /container/assign endpoint."""
import datetime

from datetime import timedelta, datetime as dt
from flask import current_app
import pytest

from app.celery.aws_ecs_creation import assign_container
from app.helpers.blueprint_helpers.aws.aws_container_post import (
    BadAppError,
    preprocess_task_info,
)


from ..patches import apply_async, function, set_stripe_customer_id
from app.helpers.utils.payment.stripe_client import StripeClient
from app.serializers.public import UserSchema


def bad_app(*args, **kwargs):
    raise BadAppError


# the following repeated monkeypatches (for userschema, stripeclient) are temporary, will need to replace with properly authenticated users later


def test_bad_app(client, authorized, monkeypatch):
    # I couldn't figure out how to patch a function that was defined at the
    # top level of a module. I found this solution at:
    # https://stackoverflow.com/questions/28403380/py-tests-monkeypatch-setattr-not-working-in-some-cases

    monkeypatch.setattr(preprocess_task_info, "__code__", bad_app.__code__)
    monkeypatch.setattr(UserSchema, "dump", set_stripe_customer_id)
    monkeypatch.setattr(StripeClient, "validate_customer_id", function(returns=True))

    response = client.post(
        "/container/assign", json=dict(username=authorized.user_id, app="Bad App")
    )

    assert response.status_code == 400


def test_no_username(client, authorized):
    response = client.post("/container/assign", json=dict(app="VSCode"))

    assert response.status_code == 401


def test_no_app(client, authorized, monkeypatch):
    monkeypatch.setattr(UserSchema, "dump", set_stripe_customer_id)
    monkeypatch.setattr(StripeClient, "validate_customer_id", function(returns=True))

    response = client.post("/container/assign", json=dict(username=authorized.user_id))

    assert response.status_code == 400


def test_no_region(client, authorized):
    response = client.post(
        "/container/assign", json=dict(username=authorized.user_id, app="VSCode")
    )


@pytest.fixture
def test_payment_required(make_authorized_user):
    def _test_payment_required(onFreeTrial, isStripeValid, client, monkeypatch):

        if onFreeTrial:
            # create a user with a time stamp less than 7 days ago
            authorized = make_authorized_user(
                client,
                monkeypatch,
                stripe_customer_id="random1234",
                created_timestamp=dt.now(datetime.timezone.utc).timestamp(),
            )
        else:
            # create a user with a timestamp of more than 7 days ago
            authorized = make_authorized_user(
                client, monkeypatch, stripe_customer_id="random1234", created_timestamp=1000000000
            )

        monkeypatch.setattr(StripeClient, "validate_customer_id", function(returns=isStripeValid))
        monkeypatch.setattr(assign_container, "apply_async", apply_async)

        response = client.post(
            "/container/assign",
            json=dict(username=authorized.user_id, app="Blender", region="us-east-1"),
        )
        return response

    return _test_payment_required


def test_successful(client, monkeypatch, test_payment_required):
    response1 = test_payment_required(True, True, client, monkeypatch)
    assert response1.status_code == 202
    response2 = test_payment_required(True, False, client, monkeypatch)
    assert response2.status_code == 202
    response3 = test_payment_required(False, True, client, monkeypatch)
    assert response3.status_code == 202
    response4 = test_payment_required(False, False, client, monkeypatch)
    assert response4.status_code == 402
