"""Fractal's Stripe utility library.

A Stripe API key must be set before the functions in this utility library may be used.

Example usage:

    from http import HTTPStatus

    import stripe

    from flask import Flask

    from payments import payment_required, PaymentRequired

    stripe.api_key = "..."
    app = Flask(__name__)


    @app.errorhandler(PaymentRequired)
    def _handle_payment_required(e):
        return (
            jsonify(error="That resource is only available to Fractal subscribers"),
            HTTPStatus.PAYMENT_REQUIRED,
        )


    @app.route("/hello")
    @payment_required
    def hello():
        return "Hello, world!"

"""

import functools
from typing import Optional
from time import time

import stripe
from flask import current_app
from flask_jwt_extended import get_jwt, verify_jwt_in_request

from auth0 import has_scope


class PaymentRequired(Exception):
    """Raised by ensure_subscribed() when a user does not have an active subscription."""


def check_payment() -> None:
    """Make sure the authenticated user has an active Fractal subscription.

    The user's subscription is considered valid if and only if it is paid or it is still in the
    trial period.

    Returns:
        None

    Raises:
        PaymentRequired: The customer has no active subscriptions.
    """

    stripe_customer_id = get_stripe_customer_id()

    if not stripe_customer_id:
        raise PaymentRequired

    customer = stripe.Customer.retrieve(stripe_customer_id, expand=("subscriptions",))

    if not any(
        subscription["status"] in ("active", "trialing")
        for subscription in customer["subscriptions"]
    ):
        raise PaymentRequired


def get_stripe_customer_id() -> Optional[str]:
    """Read the authenticated user's Stripe customer ID from their access token.

    Returns:
        The user's Stripe customer ID as a string or None if no Stripe customer ID can be found in
        the access token.
    """

    return get_jwt().get(current_app.config["STRIPE_CUSTOMER_ID_CLAIM"])


def payment_required(view_func):
    """A view decorator that prevents non-paying users from accessing Flask endpoints.

    Args:
        view_func: The Flask view function to which the decorator is applied.

    Returns:
        Another Flask view function. This one is inaccessible to users without valid Fractal
        subscriptions.
    """

    @functools.wraps(view_func)
    def wrapper(*args, **kwargs):
        start_time = time() * 1000
        verify_jwt_in_request()

        if not has_scope("admin"):
            check_payment()

        print(f"It took {time()*1000 - start_time} ms to check payment for this user")
        return view_func(*args, **kwargs)

    return wrapper
