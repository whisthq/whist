"""Tests for the /container/assign endpoint."""

import datetime

from datetime import timedelta, datetime as dt
from http import HTTPStatus

import pytest
import time


from app.celery.aws_ecs_creation import assign_container, _get_num_extra
from app.constants.time import SECONDS_IN_MINUTE, MINUTES_IN_HOUR, HOURS_IN_DAY
from app.helpers.utils.payment.stripe_client import StripeClient
from app.models import db
from app.serializers.public import UserSchema

from ..patches import function, Object


@pytest.fixture
def set_valid_subscription(monkeypatch):
    """Patch StripeClient according to whether or not the user's Stripe information is valid.

    Args:
        valid: A boolean indicating whether or not the user's Stripe information is valid.

    Returns:
        None
    """

    def _set_valid_subscription(valid):
        subscriptions = ["some_subscription"] if valid else []
        monkeypatch.setattr(StripeClient, "get_subscriptions", function(returns=subscriptions))

    return _set_valid_subscription


# the following repeated monkeypatches (for userschema, stripeclient) are temporary, will need to replace with properly authenticated users later
def test_bad_app(client, authorized, monkeypatch, set_valid_subscription):
    monkeypatch.setattr(UserSchema, "dump", function(returns={"stripe_customer_id": "random1234"}))
    set_valid_subscription(True)

    response = client.post(
        "/container/assign", json=dict(username=authorized.user_id, app="Bad App")
    )
    assert response.status_code == HTTPStatus.BAD_REQUEST


def test_no_username(client, authorized, set_valid_subscription):
    set_valid_subscription(True)

    response = client.post("/container/assign", json=dict(app="VSCode"))
    assert response.status_code == HTTPStatus.UNAUTHORIZED


def test_no_app(client, authorized, monkeypatch, set_valid_subscription):
    monkeypatch.setattr(UserSchema, "dump", function(returns={"stripe_customer_id": "random1234"}))
    set_valid_subscription(True)

    response = client.post("/container/assign", json=dict(username=authorized.user_id))

    assert response.status_code == HTTPStatus.BAD_REQUEST


def test_no_region(client, authorized, monkeypatch, set_valid_subscription):
    monkeypatch.setattr(UserSchema, "dump", function(returns={"stripe_customer_id": "random1234"}))
    set_valid_subscription(True)

    response = client.post(
        "/container/assign", json=dict(username=authorized.user_id, app="VSCode")
    )

    assert response.status_code == HTTPStatus.BAD_REQUEST


def test_get_num_extra_empty(deployment_stage):
    assert _get_num_extra(f"fractal-{deployment_stage}-browsers-chrome", "us-east-1") == 1


def test_get_num_extra_full(container, deployment_stage):
    with container(delete_quick=False, is_assigned=False) as _:
        db.session.expire_all()
        assert _get_num_extra(f"fractal-{deployment_stage}-browsers-chrome", "us-east-1") == 0


def test_get_num_extra_fractional(container, deployment_stage):
    for _ in range(15):
        _ = container(delete_quick=False, is_assigned=True)
    db.session.expire_all()
    assert _get_num_extra(f"fractal-{deployment_stage}-browsers-chrome", "us-east-1") == 3


def test_get_num_extra_subtracts(container, deployment_stage):
    for _ in range(15):
        _ = container(delete_quick=False, is_assigned=True)
    _ = container(delete_quick=False, is_assigned=False)
    db.session.expire_all()
    assert _get_num_extra(f"fractal-{deployment_stage}-browsers-chrome", "us-east-1") == 2


@pytest.fixture
def test_payment(client, make_authorized_user, monkeypatch, set_valid_subscription):
    """Generates a function to get the response of the /container/assign endpoint
    given a valid user's payment information"""

    task = Object()

    def _test_payment(onFreeTrial, isStripeValid):
        """Gets the response of the /container/assign endpoint given whether
        the user is on a free trial or has valid Stripe information

        Creates a user with the correct timestamp information and patches
        functions appropriately, then POSTs to get a response from the /container/assign endpoint
        """
        if onFreeTrial:
            # create a user with a time stamp less than 7 days ago
            authorized = make_authorized_user(
                stripe_customer_id="random1234",
                created_timestamp=dt.now(datetime.timezone.utc).timestamp(),
                domain="gmail.com",
            )
        else:
            # create a user with a timestamp of more than 7 days ago
            authorized = make_authorized_user(
                stripe_customer_id="random1234",
                created_timestamp=dt.now(datetime.timezone.utc).timestamp()
                - 7 * SECONDS_IN_MINUTE * MINUTES_IN_HOUR * HOURS_IN_DAY,
                domain="gmail.com",
            )

        task = Object()

        set_valid_subscription(isStripeValid)
        monkeypatch.setattr(assign_container, "apply_async", function(returns=task))
        monkeypatch.setattr(task, "id", "garbage-task-uuid")

        response = client.post(
            "/container/assign",
            json=dict(username=authorized.user_id, app="Blender", region="us-east-1"),
        )

        return response

    return _test_payment


@pytest.fixture
def test_payment_dev(client, make_authorized_user, monkeypatch, set_valid_subscription):
    """Generates a function to get the response of the /container/assign
    endpoint given a valid user's payment information"""

    task = Object()

    def _test_payment(onFreeTrial, isStripeValid):
        """Gets the response of the /container/assign endpoint given whether
        the user is on a free trial or has valid Stripe information

        Creates a user with the correct timestamp information and patches
        functions appropriately, then POSTs to get a response from the /container/assign endpoint
        """
        if onFreeTrial:
            # create a user with a time stamp less than 7 days ago
            authorized = make_authorized_user(
                stripe_customer_id="random1234",
                created_timestamp=dt.now(datetime.timezone.utc).timestamp(),
                domain="fractal.co",
            )
        else:
            # create a user with a timestamp of more than 7 days ago
            authorized = make_authorized_user(
                stripe_customer_id="random1234",
                created_timestamp=dt.now(datetime.timezone.utc).timestamp()
                - 7 * SECONDS_IN_MINUTE * MINUTES_IN_HOUR * HOURS_IN_DAY,
                domain="fractal.co",
            )

        task = Object()

        set_valid_subscription(isStripeValid)
        monkeypatch.setattr(assign_container, "apply_async", function(returns=task))
        monkeypatch.setattr(task, "id", "garbage-task-uuid")

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

    Tests the @payment_required decorator with whether a user
    has/does not have a free Fractal trial and has/does not have a subscription
    """
    assert test_payment(has_trial, has_subscription).status_code == expected


@pytest.mark.parametrize(
    "has_trial, has_subscription, expected",
    [
        (True, True, HTTPStatus.ACCEPTED),
        (True, False, HTTPStatus.ACCEPTED),
        (False, True, HTTPStatus.ACCEPTED),
        (False, False, HTTPStatus.ACCEPTED),
    ],
)
def test_payment_dev_all(test_payment_dev, has_trial, has_subscription, expected):
    """Tests all four cases of a user's possible payment information for expected behavior

    Tests the @payment_required decorator with whether a user
    has/does not have a free Fractal trial and has/does not have a subscription
    """
    assert test_payment_dev(has_trial, has_subscription).status_code == expected
