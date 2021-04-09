"""Unit tests for the User model and its serializer."""

import uuid

import pytest
import stripe

from app.models import db, User
from app.serializers.public import UserSchema

from tests.patches import function


def test_serialize(make_user):
    """Serialize an instance of the user model correctly.

    Instead of a created_at key of type str in the serializer's output dictionary, there should be
    a created_timestamp key of type int.
    """

    user = make_user()
    schema = UserSchema()
    user_dict = schema.dump(user)

    assert "created_at" not in user_dict
    assert type(user_dict["created_timestamp"]) == int


@pytest.mark.parametrize("stripe_customer_id", (None, ""))
def test_no_stripe_customer_id(make_user, stripe_customer_id):
    """Report that users with various falsy Stripe customer IDs should not receive service."""

    user = make_user(stripe_customer_id=stripe_customer_id)

    assert not user.subscribed


@pytest.mark.parametrize(
    "subscription_status, should_receive_service",
    (
        ("active", True),
        ("canceled", False),
        ("incomplete", False),
        ("incomplete_expired", False),
        ("past_due", False),
        ("trialing", True),
        ("unpaid", False),
    ),
)
def test_stripe_customer_id(make_user, monkeypatch, should_receive_service, subscription_status):
    """Report that a user with an active Stripe subscription should receive service."""

    customer = {"subscriptions": {"data": [{"status": subscription_status}]}}
    user = make_user(stripe_customer_id="garbage")

    monkeypatch.setattr(stripe.Customer, "retrieve", function(returns=customer))

    assert user.subscribed == should_receive_service


def test_register_user(client):
    """Ensure that new users are immediately granted access to Fractal."""

    email = f"test-{uuid.uuid4()}@example.com"
    response = client.post(
        "/account/register",
        json={
            "username": email,
            "password": "",
            "encrypted_config_token": "blah",
            "name": "Name",
            "feedback": "Testing",
        },
    )

    assert response.status_code == 200

    user = User.query.filter_by(user_id=email).one()

    assert user.subscribed

    stripe.Customer.delete(user.stripe_customer_id)
    db.session.delete(user)
    db.session.commit()
