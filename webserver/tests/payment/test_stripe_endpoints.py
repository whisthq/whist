"""Tests for stripe endpoints."""

import os
from http import HTTPStatus
from typing import Callable, Optional

import pytest
import stripe
from flask import current_app
from pytest import MonkeyPatch

from tests.patches import function, Object
from tests.client import FractalAPITestClient


@pytest.mark.parametrize("subscription_status", (None, "canceled", "incomplete_expired", "unpaid"))
def test_payment_portal_no_subscription(
    client: FractalAPITestClient,
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
        "app.utils.stripe.payments.get_subscription_status", function(returns=subscription_status)
    )
    monkeypatch.setattr(stripe.billing_portal.Session, "create", function(raises=Exception))
    monkeypatch.setattr(stripe.checkout.Session, "create", function(returns=checkout_session))
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


@pytest.mark.parametrize("subscription_status", ("active", "past_due", "incomplete", "trialing"))
def test_create_billing_portal_unauthorized(
    client: FractalAPITestClient,
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
        "app.utils.stripe.payments.get_subscription_status", function(returns=subscription_status)
    )
    monkeypatch.setattr(stripe.billing_portal.Session, "create", function(returns=portal_session))
    monkeypatch.setattr(stripe.checkout.Session, "create", function(raises=Exception))
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
