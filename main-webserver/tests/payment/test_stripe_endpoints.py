"""Tests for stripe endpoints."""

import pytest
import requests
import stripe

from ..patches import function, Object
from ..helpers.general.progress import queryStatus

stripe.api_key = "sk_test_6ndCgv5edtzMuyqMoBbt1gXj00xy90yd4L"


def test_expired_subscription(client):
    """Handle expired subscription."""

    payment_method = stripe.PaymentMethod.create(
        type="card",
        card={
            "number": "4242424242424242",
            "exp_month": 5,
            "exp_year": 2022,
            "cvc": "314",
        },
    )
    customer = stripe.Customer.create(
        description="Test Customer",
    )

    product = stripe.Product.list()["data"][0]["id"]

    price = stripe.Price.list()["data"][0]["id"]

    subscription = stripe.Subscription.create(
        customer=customer["id"],
        items=[
            {"price": price},
        ],
        trial_end="now",
    )
    resp = client.post(
        "/stripe/can_access_product",
        json=dict(stripe_id=customer["id"]),
    )

    task = queryStatus(client, resp, timeout=10)

    if task["status"] < 1:
        assert False

    assert task["result"]["subscribed"]
