"""Tests for stripe endpoints."""

import pytest
import requests
import stripe

from ..patches import function, Object
from ..helpers.general.progress import queryStatus


@pytest.mark.usefixtures("authorized")
def test_active_subscription(client):
    """Handle active subscription."""
    # create test subscription
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

    # verify endpoint works
    resp = client.post(
        "/stripe/can_access_product",
        json=dict(stripe_id=customer["id"]),
    )

    # delete test subscription
    stripe.Subscription.delete(subscription["id"])
    stripe.Price.modify(price["id"], active="False")
    stripe.Product.modify(product["id"], active="False")
    stripe.Customer.delete(customer["id"])

    assert resp.json["subscribed"]
