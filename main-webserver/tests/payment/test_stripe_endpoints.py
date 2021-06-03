"""Tests for stripe endpoints."""

from datetime import datetime
from dateutil.relativedelta import relativedelta

import pytest
import stripe

from app.helpers.utils.general.time import date_to_unix
from app.constants.http_codes import SUCCESS, BAD_REQUEST


"""Here we are the fixtures that we use to make Stripe customers to test with. These can be used to create prices, products, subscriptions, etc.
"""


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
    """Returns a UNIX date a week from now"""
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


"""Here are the tests for the get_customer_status helper function.
"""

# TODO: Replace the following with tests testing the get_customer_status helper

# @pytest.mark.usefixtures("authorized")
# def test_cannot_access_product(client, customer):
#     """Test the /stripe/can_access_product endpoint"""
#     response = client.post(
#         "/stripe/can_access_product",
#         json=dict(customer_id=customer["id"]),
#     )
#     print(response.json)
#     assert response.status_code == SUCCESS
#     assert not response.json["subscribed"]


# @pytest.mark.skip
# @pytest.mark.usefixtures("authorized")
# @pytest.mark.usefixtures("subscription")
# def test_can_access_product(client, customer):
#     """Test the /stripe/can_access_product endpoint

#     This is different than the other because it uses the subscription fixture.
#     """
#     response = client.post(
#         "/stripe/can_access_product",
#         json=dict(),
#     )
#     assert response.status_code == BAD_REQUEST

#     response = client.post(
#         "/stripe/can_access_product",
#         json=dict(customer_id=""),
#     )
#     assert response.status_code == BAD_REQUEST

#     response = client.post(
#         "/stripe/can_access_product",
#         json=dict(customer_id=customer["id"]),
#     )
#     assert response.status_code == SUCCESS
#     assert response.json["subscribed"]


"""Here are the tests for the /stripe/create_checkout_session endpoint.
"""


@pytest.mark.skip
@pytest.mark.usefixtures("authorized")
def test_checkout_portal_no_price(client):
    """Test the /stripe/create_checkout_session endpoint with no arguments"""
    response = client.post(
        "/stripe/create_checkout_session",
        json=dict(),
    )
    assert response.status_code == BAD_REQUEST


@pytest.mark.skip
@pytest.mark.usefixtures("authorized")
def test_create_checkout_portal(client, customer, price):
    """Test the /stripe/create_checkout_session endpoint"""
    response = client.post(
        "/stripe/create_checkout_session",
        json=dict(
            customer_id=customer["id"],
            success_url="https://fractal.co",
            cancel_url="https://fractal.co",
        ),
    )
    print(response.json)
    assert response.status_code == SUCCESS
    assert response.json["session_id"]


"""Here are the tests for the /stripe/customer_portal endpoint.
"""


@pytest.mark.skip
@pytest.mark.usefixtures("authorized")
def test_create_billing_portal(client, customer):
    """Test the /stripe/customer_portal endpoint"""
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
