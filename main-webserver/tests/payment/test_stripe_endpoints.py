"""Tests for stripe endpoints."""

import pytest
import stripe

from datetime import datetime
from dateutil.relativedelta import relativedelta
from app.helpers.utils.general.time import date_to_unix

from app.constants.http_codes import SUCCESS, BAD_REQUEST


@pytest.fixture(scope="session")
def customer():
    """Creates a Stripe customer"""
    stripe_customer = stripe.Customer.create(
        description="Test Customer",
    )
    stripe.Customer.create_source(stripe_customer["id"], source="tok_visa")
    yield stripe_customer

    stripe.Customer.delete(stripe_customer["id"])


@pytest.fixture(scope="session")
def product():
    """Creates a Stripe product"""
    product = stripe.Product.create(name="Test Subscription")

    yield product

    stripe.Product.modify(product["id"], active="False")


@pytest.fixture(scope="session")
def price(product):
    """Creates a Stripe price for a given product

    Args:
        unit_amount (number): unit price of the product
        currency (str): currency of the unit price
        recurring (JSON): JSON in the format {"interval": ""}, where interval is the recurrence frequency
        product (obj): Stripe product to apply this price to
    """

    price = stripe.Price.create(
        unit_amount=50,
        currency="usd",
        recurring={"interval": "month"},
        product=product["id"],
    )

    yield price

    stripe.Price.modify(price["id"], active="False")


@pytest.fixture(scope="session")
def trial_end():
    return date_to_unix(datetime.now() + relativedelta(weeks=1))


@pytest.fixture(scope="session")
def subscription(customer, price, trial_end):
    """Creates a Stripe subscription

    Args:
        customer (obj): Stripe customer to create this subscription for
        price (obj): Stripe price to subscribe to
        trial_end (string): end time for this subscription
    """

    subscription = stripe.Subscription.create(
        customer=customer["id"],
        items=[
            {"price": price["id"]},
        ],
        trial_end=trial_end,
    )

    yield subscription

    stripe.Subscription.delete(subscription["id"])


# @pytest.mark.usefixtures("authorized")
# def test_active_subscription(client):
#     """Handle active subscription."""
#     # create test subscription
#     customer = stripe.Customer.create(
#         description="Test Customer",
#     )
#     stripe.Customer.create_source(customer["id"], source="tok_visa")

#     product = stripe.Product.create(name="Test Subscription")
#     price = stripe.Price.create(
#         unit_amount=50,
#         currency="usd",
#         recurring={"interval": "month"},
#         product=product["id"],
#     )
#     subscription = stripe.Subscription.create(
#         customer=customer["id"],
#         items=[
#             {"price": price["id"]},
#         ],
#         trial_end="now",
#     )

#     # verify endpoint works
#     resp = client.post(
#         "/stripe/can_access_product",
#         json=dict(stripe_id=customer["id"]),
#     )

#     # delete test subscription
#     stripe.Subscription.delete(subscription["id"])
#     stripe.Price.modify(price["id"], active="False")
#     stripe.Product.modify(product["id"], active="False")
#     stripe.Customer.delete(customer["id"])

#     assert resp.json["subscribed"]


@pytest.mark.usefixtures("authorized")
def test_cannot_access_product(client, customer):
    response = client.post(
        "/stripe/can_access_product",
        json=dict(customer_id=customer["id"]),
    )
    assert response.status_code == SUCCESS
    assert not response.json["subscribed"]


@pytest.mark.usefixtures("authorized")
@pytest.mark.usefixtures("subscription")
def test_can_access_product(client, customer):
    response = client.post(
        "/stripe/can_access_product",
        json=dict(),
    )
    assert response.status_code == BAD_REQUEST

    response = client.post(
        "/stripe/can_access_product",
        json=dict(customer_id=""),
    )
    assert response.status_code == BAD_REQUEST

    response = client.post(
        "/stripe/can_access_product",
        json=dict(customer_id=customer["id"]),
    )
    assert response.status_code == SUCCESS
    assert response.json["subscribed"]


@pytest.mark.usefixtures("authorized")
def test_checkout_portal_no_price(client):
    response = client.post(
        "/stripe/create_checkout_session",
        json=dict(),
    )
    assert response.status_code == BAD_REQUEST


@pytest.mark.usefixtures("authorized")
def test_create_checkout_portal(client, customer, price):
    response = client.post(
        "/stripe/create_checkout_session",
        json=dict(
            customer_id=customer["id"],
            price_id=price["id"],
            success_url="https://fractal.co",
            cancel_url="https://fractal.co",
        ),
    )
    print(response.json)
    assert response.status_code == SUCCESS
    assert response.json["session_id"]


@pytest.mark.usefixtures("authorized")
def test_create_billing_portal(client, customer):
    response = client.post(
        "/stripe/customer_portal",
        json=dict(),
    )
    assert response.status_code == BAD_REQUEST

    response = client.post(
        "/stripe/customer_portal",
        json=dict(customer_id=customer["id"], return_url="https://fractal.co"),
    )
    assert response.status_code == SUCCESS
    assert response.json["url"]


def test_stripe_webhook(client):
    response = client.post(
        "/stripe/webhook",
        json=dict(type="invoice.created"),
    )
    assert response.status_code == SUCCESS
