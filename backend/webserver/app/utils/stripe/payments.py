"""Whist's Stripe utility library.

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
        return jsonify(error="That resource is only available to Whist subscribers"), 402

    @app.route("/hello")
    @payment_required
    def hello():
        return "Hello, world!"

"""

import functools
import logging
from http import HTTPStatus
from typing import Any, Callable, cast, Optional, Dict, List
from time import time

import stripe
from flask import current_app, jsonify
from flask_jwt_extended import get_jwt, verify_jwt_in_request

from app.utils.auth.auth0 import has_scope

logger = logging.getLogger(__name__)


class PaymentRequired(Exception):
    """Raised by ensure_subscribed() when a user does not have an active subscription."""


def check_payment() -> None:
    """Make sure the authenticated user has an active Whist subscription.

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

        If the authenticated user already has a Whist subscription in a non-terminal state (e.g.
        one of ``active``, ``trialing``, ``incomplete``, or ``past_due``), create a Stripe customer
        portal session that the customer can use to manage their subscription and billing
        information.

        If the authenticated user has no Whist subscriptions in non-terminal states (i.e. all of
        the user's subscriptions are ``canceled``, ``unpaid``, or ``incomplete_expired``), create a
        Stripe checkout session that the customer can use to enroll in a new Whist subscription.

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

        subscription_status = get_stripe_subscription_status(customer)

        try:
            if not subscription_status in ["active", "trialing"]:
                # Fetch the Stripe Price that matches our currently desired price, or create
                # a new Price if no Price matches
                desired_price = int(current_app.config["MONTHLY_PRICE_IN_DOLLARS"])
                price_id = next(
                    (
                        [
                            price["id"]
                            for price in list_all_stripe_prices()
                            if price["unit_amount"] == desired_price
                        ]
                    ),
                    create_price(desired_price)["id"],
                )

                # Any subscriptions that is not active or in the free trial period means that the user
                # should re-enter their payment info for a brand-new subscription
                session = stripe.checkout.Session.create(
                    customer=customer,
                    line_items=[
                        {
                            "price": price_id,
                            "quantity": 1,
                        },
                    ],
                    subscription_data={"trial_period_days": 14},
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
        except stripe.error.InvalidRequestError as e:
            logger.error(e)
            logger.error(f"Stripe returned an error. Does customer {customer} exist?")

            return (
                jsonify(error="Unable to locate your customer record"),
                HTTPStatus.INTERNAL_SERVER_ERROR,
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


def list_all_stripe_prices() -> List[Dict[str, any]]:
    """Returns all available Stripe Prices

    Returns:
        A list of Stripe Prices
    """

    try:
        # The Stripe API returns a maximum of 10 results at a time, so we need to loop in case
        # there are more than 10 active prices
        should_fetch_more_results = True
        prices = []

        while should_fetch_more_results:
            prices += stripe.Price.list().data
            should_fetch_more_results = prices.has_more

        return prices
    except stripe.error.InvalidRequestError as e:
        logger.error(e)
    except IndexError:
        logger.warning("There are no prices active in Stripe")


def get_stripe_subscription_status(customer_id: str) -> Optional[str]:
    """Attempt to get subscription status from stripe but fallback to access token

    Returns:
        A string containing the value of the status attribute of the Stripe subscription record
        representing the user's most recently created subscription or None if the user has no
        non-cancelled subscriptions or the subscription status claim is missing. See
        https://stripe.com/docs/api/subscriptions/object#subscription_object-status.
    """

    try:
        customer = stripe.Subscription.list(
            customer=customer_id, current_period_end={"gt": int(time())}
        )

        # Cast to Optional[str] to please mypy.
        return cast(Optional[str], customer["data"][0]["status"])
    except stripe.error.InvalidRequestError as e:
        logger.error(e)
    except IndexError:
        logger.warning("Stripe reports no current subscriptions for customer {customer_id}")

    # Fall back to reading customer's subscription status from their access token.
    return get_subscription_status()


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


def create_price(amount: int) -> Dict[str, any]:
    """Creates a new Stripe Price with the desired monthly recurring amount

    Returns:
        The newly-created Stripe Price
    """
    return stripe.Price.create(
        unit_amount=amount * 100,
        currency="usd",
        recurring={"interval": "month"},
        product_data={"name": "Whist (Monthly)"},
    )


def payment_required(view_func: Callable[..., Any]) -> Callable[..., Any]:
    """A view decorator that prevents non-paying users from accessing Flask endpoints.

    Args:
        view_func: The Flask view function to which the decorator is applied.

    Returns:
        Another Flask view function. This one is inaccessible to users without valid Whist
        subscriptions.
    """

    @functools.wraps(view_func)
    def wrapper(*args: Any, **kwargs: Any) -> Any:
        verify_jwt_in_request()

        if not has_scope("admin"):
            check_payment()

        return view_func(*args, **kwargs)

    return wrapper
