"""Tests for stripe endpoints."""

import pytest
import requests
import stripe

from ..patches import function, Object
from ..helpers.general.progress import queryStatus


@pytest.mark.usefixtures("authorized")
def test_expired_subscription(client):
    """Handle expired subscription."""

    customer = stripe.Customer.create(
        description="Test Customer",
    )

    stripe.Customer.create_source(customer["id"], source="tok_visa")

    product = stripe.Product.create(name="Test Subscription")

    price = stripe.Price.create(
        unit_amount=50,
        currency="usd",
        recurring={"interval": "month"},
        product=product["id"],
    )

    subscription = stripe.Subscription.create(
        customer=customer["id"],
        items=[
            {"price": price["id"]},
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
