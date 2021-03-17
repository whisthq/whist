"""Tests for the /container/assign endpoint."""

from datetime import datetime, timedelta, timezone
from http import HTTPStatus

import pytest


from app.celery.aws_ecs_creation import assign_container, _get_num_extra
from app.helpers.utils.payment.stripe_client import StripeClient
from app.models import SupportedAppImages
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


@pytest.mark.usefixtures("authorized")
def test_no_username(client, set_valid_subscription):
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


def test_get_num_extra_empty(task_def_env):
    # tests the base case of _get_num_extra -- prewarm 1
    assert _get_num_extra(f"fractal-{task_def_env}-browsers-chrome", "us-east-1") == 1


def test_get_num_extra_full(bulk_container, task_def_env):
    # tests the buffer filled case of _get_num_extra -- 1 already prewarmed
    _ = bulk_container(is_assigned=False)
    assert _get_num_extra(f"fractal-{task_def_env}-browsers-chrome", "us-east-1") == 0


def test_get_num_extra_fractional(bulk_container, task_def_env):
    # tests the fractional case of _get_num_extra, ensure the ratio is right
    for _ in range(15):
        _ = bulk_container(is_assigned=True)
    preboot_num = (
        SupportedAppImages.query.filter_by(
            task_definition=f"fractal-{task_def_env}-browsers-chrome"
        )
        .first()
        .preboot_number
    )
    assert (
        _get_num_extra(f"fractal-{task_def_env}-browsers-chrome", "us-east-1") == 15.0 * preboot_num
    )


def test_get_num_extra_subtracts(bulk_container, task_def_env):
    # tests the total codepath set of _get_num_extra
    # fractional reserve and buffer filling
    for _ in range(15):
        _ = bulk_container(is_assigned=True)
    _ = bulk_container(is_assigned=False)
    preboot_num = (
        SupportedAppImages.query.filter_by(
            task_definition=f"fractal-{task_def_env}-browsers-chrome"
        )
        .first()
        .preboot_number
    )
    assert (
        _get_num_extra(f"fractal-{task_def_env}-browsers-chrome", "us-east-1")
        == (15.0 * preboot_num) - 1
    )


@pytest.fixture
def test_payment(client, make_authorized_user, monkeypatch, set_valid_subscription):
    """Generates a function to get the response of the /container/assign endpoint
    given a valid user's payment information"""

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
                domain="gmail.com",
            )
        else:
            # create a user with a timestamp of more than 7 days ago
            authorized = make_authorized_user(
                stripe_customer_id="random1234",
                created_at=datetime.now(timezone.utc) - timedelta(weeks=1),
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
                domain="fractal.co",
            )
        else:
            # create a user with a timestamp of more than 7 days ago
            authorized = make_authorized_user(
                stripe_customer_id="random1234",
                created_at=datetime.now(timezone.utc) - timedelta(weeks=1),
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
