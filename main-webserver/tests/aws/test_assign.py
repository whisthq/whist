"""Tests for the /container/assign endpoint."""

import datetime

from datetime import timedelta, datetime as dt
from http import HTTPStatus

import pytest

from flask import current_app

from app.celery.aws_ecs_creation import assign_container
from app.constants.time import SECONDS_IN_MINUTE, MINUTES_IN_HOUR, HOURS_IN_DAY
from app.helpers.blueprint_helpers.aws.aws_container_post import (
    BadAppError,
    preprocess_task_info,
)
from app.helpers.utils.payment.stripe_client import StripeClient
from app.serializers.public import UserSchema

from ..patches import apply_async, function


def bad_app(*args, **kwargs):
    raise BadAppError


def set_valid_subscription(monkeypatch, valid):
    subscriptions = ["some_subscription"] if valid else []
    monkeypatch.setattr(StripeClient, "get_subscriptions", function(returns=subscriptions))


# the following repeated monkeypatches (for userschema, stripeclient) are temporary, will need to replace with properly authenticated users later
def test_bad_app(client, authorized, monkeypatch):
    # I couldn't figure out how to patch a function that was defined at the
    # top level of a module. I found this solution at:
    # https://stackoverflow.com/questions/28403380/py-tests-monkeypatch-setattr-not-working-in-some-cases

    monkeypatch.setattr(preprocess_task_info, "__code__", bad_app.__code__)
    monkeypatch.setattr(UserSchema, "dump", function(returns={"stripe_customer_id": "random1234"}))
    set_valid_subscription(monkeypatch, True)

    response = client.post(
        "/container/assign", json=dict(username=authorized.user_id, app="Bad App")
    )
    assert response.status_code == HTTPStatus.BAD_REQUEST


def test_no_username(client, authorized, monkeypatch):
    set_valid_subscription(monkeypatch, True)

    response = client.post("/container/assign", json=dict(app="VSCode"))
    assert response.status_code == HTTPStatus.UNAUTHORIZED


def test_no_app(client, authorized, monkeypatch):
    monkeypatch.setattr(UserSchema, "dump", function(returns={"stripe_customer_id": "random1234"}))
    set_valid_subscription(monkeypatch, True)

    response = client.post("/container/assign", json=dict(username=authorized.user_id))

    assert response.status_code == HTTPStatus.BAD_REQUEST


def test_no_region(client, authorized, monkeypatch):
    monkeypatch.setattr(UserSchema, "dump", function(returns={"stripe_customer_id": "random1234"}))
    set_valid_subscription(monkeypatch, True)

    response = client.post(
        "/container/assign", json=dict(username=authorized.user_id, app="VSCode")
    )

    assert response.status_code == HTTPStatus.BAD_REQUEST


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
                client,
                monkeypatch,
                stripe_customer_id="random1234",
                created_timestamp=dt.now(datetime.timezone.utc).timestamp()
                - 7 * SECONDS_IN_MINUTE * MINUTES_IN_HOUR * HOURS_IN_DAY,
            )

        set_valid_subscription(monkeypatch, isStripeValid)
        monkeypatch.setattr(assign_container, "apply_async", apply_async)

        response = client.post(
            "/container/assign",
            json=dict(username=authorized.user_id, app="Blender", region="us-east-1"),
        )
        return response

    return _test_payment


@pytest.mark.parametrize(
    "has_trial, has_subscription, expected",
    [
        (True, True, HTTPStatus.ACCEPTED),
        (True, False, HTTPStatus.ACCEPTED),
        (False, True, HTTPStatus.ACCEPTED),
        (False, False, HTTPStatus.PAYMENT_REQUIRED),
    ],
)
def test_payment_all(test_payment, has_trial, has_subscription, expected):
    """Tests all four cases of a user's possible payment information for expected behavior

    Tests the @payment_required decorator with whether a user has/does not have a free Fractal trial and has/does not have a subscription
    """
    assert test_payment(has_trial, has_subscription).status_code == expected
