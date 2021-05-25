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

import stripe
from flask_jwt_extended import get_jwt, verify_jwt_in_request

from auth0 import has_scope


class PaymentRequired(Exception):
    """Raised by ensure_subscribed() when a user does not have an active subscription."""


def check_payment(customer_id: str) -> None:
    """Make sure the specified user has an active Fractal subscription.

    The user's subscription is considered valid if and only if it is paid or it is still in the
    trial period.

    Args:
        customer_id: The Stripe customer ID of the customer whose subscription should be checked.

    Returns:
        None

    Raises:
        PaymentRequired: The customer has no active subscriptions.
    """

    if not customer_id:
        raise PaymentRequired

    customer = stripe.Customer.retrieve(customer_id, expand=("subscriptions",))

    if not any(
        subscription["status"] in ("active", "trialing")
        for subscription in customer["subscriptions"]
    ):
        raise PaymentRequired


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
        if not has_scope("admin"):
            verify_jwt_in_request()
            check_payment(get_jwt().get("https://api.fractal.co/stripe_customer_id"))

        return view_func(*args, **kwargs)

    return wrapper
