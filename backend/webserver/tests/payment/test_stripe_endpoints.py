"""Tests for stripe endpoints."""

import os
from http import HTTPStatus
from typing import Callable, Optional

import pytest
import stripe
from flask import current_app
from pytest import MonkeyPatch

from tests.patches import function, Object
from tests.client import WhistAPITestClient


@pytest.mark.parametrize(
    "subscription_status",
    (None, "canceled", "incomplete_expired", "unpaid", "past_due", "incomplete"),
)
def test_create_checkout_session(
    client: WhistAPITestClient,
    make_user: Callable[[], str],
    monkeypatch: MonkeyPatch,
    subscription_status: Optional[str],
) -> None:
    """Ensure that /payment_portal_url creates a checkout session for non-subscribers."""

    checkout_session = Object()
    url = f"http://localhost/{os.urandom(8).hex()}"
    user = make_user()

    monkeypatch.setattr(checkout_session, "url", url)
    monkeypatch.setattr(
        stripe.Subscription, "list", function(returns={"data": [{"status": subscription_status}]})
    )
    monkeypatch.setattr(
        "app.utils.stripe.payments.get_subscription_status",
        function(raises=Exception("This function should not have been called")),
    )
    monkeypatch.setattr(
        stripe.billing_portal.Session,
        "create",
        function(raises=Exception("Should have created a checkout session for new customer")),
    )
    monkeypatch.setattr(stripe.checkout.Session, "create", function(returns=checkout_session))
    client.login(
        user,
        admin=False,
        **{
            current_app.config["STRIPE_CUSTOMER_ID_CLAIM"]: "cus_test",
            current_app.config["STRIPE_SUBSCRIPTION_STATUS_CLAIM"]: None,
        },
    )

    response = client.get("/payment_portal_url")

    assert response.status_code == HTTPStatus.OK
    assert response.json == {"url": url}


@pytest.mark.parametrize("subscription_status", ("active", "trialing"))
def test_create_billing_portal_session(
    client: WhistAPITestClient,
    make_user: Callable[..., str],
    monkeypatch: MonkeyPatch,
    subscription_status: Optional[str],
) -> None:
    """Ensure that /payment_portal_url creates a customer portal session for subscribers."""

    portal_session = Object()
    url = f"http://localhost/{os.urandom(8).hex()}"
    user = make_user()

    monkeypatch.setattr(portal_session, "url", url)
    monkeypatch.setattr(
        stripe.Subscription, "list", function(returns={"data": [{"status": subscription_status}]})
    )
    monkeypatch.setattr(
        "app.utils.stripe.payments.get_subscription_status",
        function(raises=Exception("This function should not have been called")),
    )
    monkeypatch.setattr(stripe.billing_portal.Session, "create", function(returns=portal_session))
    monkeypatch.setattr(
        stripe.checkout.Session,
        "create",
        function(
            raises=Exception("Should have created a billing portal session for existing customer")
        ),
    )
    client.login(
        user,
        admin=False,
        **{
            current_app.config["STRIPE_CUSTOMER_ID_CLAIM"]: "cus_test",
            current_app.config["STRIPE_SUBSCRIPTION_STATUS_CLAIM"]: subscription_status,
        },
    )

    response = client.get("/payment_portal_url")

    assert response.status_code == HTTPStatus.OK
    assert response.json == {"url": url}


def test_create_price(
    client: WhistAPITestClient,
    make_user: Callable[[], str],
    monkeypatch: MonkeyPatch,
) -> None:
    """Ensure that no new Stripe Price is created if one already exists that matches our desired amount"""

    checkout_session = Object()
    url = f"http://localhost/{os.urandom(8).hex()}"
    user = make_user()

    monkeypatch.setattr(checkout_session, "url", url)
    monkeypatch.setattr(stripe.Subscription, "list", function(returns={"data": [{"status": None}]}))
    monkeypatch.setattr(
        stripe.Price, "list", function(returns={"data": [{"unit_amount": 50 * 100}]})
    )
    monkeypatch.setattr(
        stripe.Price,
        "create",
        function(raises=Exception("Price already exists, new price should not have been created")),
    )
    monkeypatch.setattr(stripe.checkout.Session, "create", function(returns=checkout_session))
    client.login(
        user,
        admin=False,
        **{
            current_app.config["STRIPE_CUSTOMER_ID_CLAIM"]: "cus_test",
            current_app.config["STRIPE_SUBSCRIPTION_STATUS_CLAIM"]: None,
        },
    )

    response = client.get("/payment_portal_url")

    assert response.status_code == HTTPStatus.OK
    assert response.json == {"url": url}


def test_missing_customer_record(client, make_user, monkeypatch):  # type: ignore[no-untyped-def]
    """Ensure that the server returns a descriptive error when it can't find a customer record."""

    user = make_user()

    monkeypatch.setattr(
        stripe.Subscription,
        "list",
        function(raises=stripe.error.InvalidRequestError("No such customer", "id")),
    )

    client.login(
        user,
        admin=False,
        **{
            current_app.config["STRIPE_CUSTOMER_ID_CLAIM"]: "cus_test",
            current_app.config["STRIPE_SUBSCRIPTION_STATUS_CLAIM"]: "active",
        },
    )

    response = client.get("/payment_portal_url")

    assert response.status_code == HTTPStatus.INTERNAL_SERVER_ERROR
    assert "error" in response.json
