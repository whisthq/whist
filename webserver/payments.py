"""Fractal's Stripe utility library.

A Stripe API key must be set before the functions in this utility library may be used.

Example usage::

    # wsgi.py

    import stripe
    from flask import Flask

    from payments import (
        customer_portal_route_factory, get_customer_id, payment_required, PaymentRequired
    )

    stripe.api_key = "..."
    app = Flask(__name__)

    app.add_url_rule(
        "/payment_portal_url",
        view_func=jwt_required()(customer_portal_route_factory(get_customer_id)),
    )

    @app.errorhandler(PaymentRequired)
    def _handle_payment_required(e):
        return jsonify(error="That resource is only available to Fractal subscribers"), 402

    @app.route("/hello")
    @payment_required
    def hello():
        return "Hello, world!"

"""

import functools
from typing import Any, Callable, Optional

import stripe
from flask import current_app, jsonify
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


def payment_portal_factory(customer_id: Callable[[], Optional[str]]) -> Callable[..., Any]:
    """Generate a view that creates a Stripe checkout or customer portal session for a user.

    The user for whom the checkout or customer portal session is created is the user returned by
    the customer_id callable.

    Args:
        customer_id: A niladic function that returns the ID of the Stripe customer for whom the
            checkout or customer portal session should be created.

    Returns:
        A Flask view function.
    """

    def customer_portal(*_args: Any, **_kwargs: Any) -> Any:
        """Create a Stripe checkout or customer portal session.

        If the authenticated user already has a Fractal subscription in a non-terminal state (e.g.
        one of ``active``, ``trialing``, ``incomplete``, or ``past_due``), create a Stripe customer
        portal session that the customer can use to manage their subscription and billing
        information.

        If the authenticated user has no Fractal subscriptions in non-terminal states (i.e. all of
        the user's subscriptions are ``canceled``, ``unpaid``, or ``incomplete_expired``), create a
        Stripe checkout session that the customer can use to enroll in a new Fractal subscription.

        See https://stripe.com/docs/api/subscriptions/object#subscription_object-status for Stripe
        subscription status documentation.

        Returns:
            A JSON object containing a single key "url", whose value is the URL of the new checkout
            customer portal session.
        """

        customer = customer_id()

        # If the Stripe customer ID is not present in the access token, something has gone horribly
        # wrong.
        assert customer

        if get_subscription_status() in (None, "canceled", "incomplete_expired", "unpaid"):
            # Any subscriptions that the user might have had are now in terminal states (e.g.
            # "canceled", "unpaid", or "incomplete_expired") and cannot be renewed. Create a
            # checkout session so the user can enroll in a new subscription.
            session = stripe.checkout.Session.create(
                customer=customer,
                line_items=[
                    {
                        "price": current_app.config["STRIPE_PRICE_ID"],
                        "quantity": 1,
                    },
                ],
                payment_method_types=("card",),
                success_url="http://localhost/callback/payment?success=true",
                cancel_url="http://localhost/callback/payment?success=false",
                mode="subscription",
            )
        else:
            # The user has a subscription in a non-terminal state (e.g. "active", "trialing", or
            # "incomplete"). Create a customer portal session so the user can update their billing
            # and subscription information.
            session = stripe.billing_portal.Session.create(
                customer=customer,
                return_url="http://localhost/callback/payment",
            )

        return jsonify(url=session.url)

    return customer_portal


def get_customer_id() -> Optional[str]:
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


def payment_required(view_func: Callable[..., Any]) -> Callable[..., Any]:
    """A view decorator that prevents non-paying users from accessing Flask endpoints.

    Args:
        view_func: The Flask view function to which the decorator is applied.

    Returns:
        Another Flask view function. This one is inaccessible to users without valid Fractal
        subscriptions.
    """

    @functools.wraps(view_func)
    def wrapper(*args: Any, **kwargs: Any) -> Any:
        verify_jwt_in_request()

        if not has_scope("admin"):
            check_payment()

        return view_func(*args, **kwargs)

    return wrapper
