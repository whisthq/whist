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


from ..patches import apply_async, function
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
    monkeypatch.setattr(UserSchema, "dump", function(returns={"stripe_customer_id": "random1234"}))
    monkeypatch.setattr(StripeClient, "validate_customer_id", function(returns=True))

    response = client.post(
        "/container/assign", json=dict(username=authorized.user_id, app="Bad App")
    )
    assert response.status_code == 400


def test_no_username(client, authorized):

    response = client.post("/container/assign", json=dict(app="VSCode"))
    assert response.status_code == 401


def test_no_app(client, authorized, monkeypatch):
    monkeypatch.setattr(UserSchema, "dump", function(returns={"stripe_customer_id": "random1234"}))
    monkeypatch.setattr(StripeClient, "validate_customer_id", function(returns=True))

    response = client.post("/container/assign", json=dict(username=authorized.user_id))

    assert response.status_code == 400


def test_no_region(client, authorized):
    response = client.post(
        "/container/assign", json=dict(username=authorized.user_id, app="VSCode")
    )


@pytest.fixture
def test_payment(client, make_authorized_user, monkeypatch):
    """Generates a function to get the response of the /container/assign endpoint given a valid user's payment information"""

    def _test_payment(onFreeTrial, isStripeValid):
        """Gets the response of the /container/assign endpoint given whether the user is on a free trial or has valid Stripe information

        Creates a user with the correct timestamp information and patches functions appropriately, then POSTs to get a response from the /container/assign endpoint
        """
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

    return _test_payment


@pytest.mark.parametrize(
    "has_trial, has_subscription, expected",
    [(True, True, 202), (True, False, 202), (False, True, 202), (False, False, 402)],
)
def test_payment_all(test_payment, has_trial, has_subscription, expected):
    """Tests all four cases of a user's possible payment information for expected behavior

    Tests the @payment_required decorator with whether a user has/does not have a free Fractal trial and has/does not have a subscription
    """
    assert test_payment(has_trial, has_subscription).status_code == expected
