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

    subscription_status = get_subscription_status()

    if subscription_status not in ("active", "trialing"):
        raise PaymentRequired


def get_stripe_customer_id() -> Optional[str]:
    """Read the authenticated user's Stripe customer ID from their access token.

    Returns:
        The user's Stripe customer ID as a string or None if no Stripe customer ID can be found in
        the access token.
    """

    return get_jwt().get(  # type: ignore[no-any-return]
        current_app.config["STRIPE_CUSTOMER_ID_CLAIM"]
    )


def get_subscription_status() -> Optional[str]:
    """Read the authenticated user's Stripe subscription status from their access token.

    Returns:
        A string containing the value of the status attribute of the Stripe subscription record
        representing the user's most recently created subscription or None if the user has no
        non-cancelled subscriptions or the subscription status claim is missing. See
        https://stripe.com/docs/api/subscriptions/object#subscription_object-status.
    """

    return get_jwt().get(  # type: ignore[no-any-return]
        current_app.config["STRIPE_SUBSCRIPTION_STATUS_CLAIM"]
    )


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
        verify_jwt_in_request()

        if not has_scope("admin"):
            check_payment()

        return view_func(*args, **kwargs)

    return wrapper
