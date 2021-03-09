"""Unit tests for the Stripe subscription change event handler."""

import random
import string

import pytest

from app.blueprints.payment.stripe_blueprint import handle_subscription_update
from app.models import db

ascii_alphanumeric = string.ascii_lowercase + string.digits


def test_bad_customer_id():
    test_event = {"data": {"object": {"customer": "cus_tomer"}}}

    handle_subscription_update(test_event)


@pytest.mark.parametrize("initially_subscribed", (True, False))
@pytest.mark.parametrize(
    "subscription_status, ultimately_subscribed",
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
def test_update(initially_subscribed, make_user, subscription_status, ultimately_subscribed):
    stripe_customer_id = "cus_{''.join(random.choices(ascii_alphanumeric, k=16))}"
    user = make_user(stripe_customer_id=stripe_customer_id, subscribed=initially_subscribed)
    test_event = {
        "data": {"object": {"customer": user.stripe_customer_id, "status": subscription_status}}
    }

    handle_subscription_update(test_event)
    db.session.refresh(user)

    assert user.subscribed == ultimately_subscribed
