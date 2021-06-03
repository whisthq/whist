from typing import Tuple
from flask import current_app, jsonify

import stripe

from app.constants.http_codes import (
    BAD_REQUEST,
    SUCCESS,
)

stripe.api_key = current_app.config["STRIPE_SECRET"]


def get_checkout_id(success_url: str, cancel_url: str, customer_id: str) -> Tuple[str, int]:
    """
    Returns checkout session id from Stripe.

    The id is based on the customer_id and return urls, and is used by stripe.js
    for displaying the portal on the front-end.

    Args:
        customer_id (str): the stripe id of the user
        success_url (str): url to redirect to upon completion success
        cancel_url (str): url to redirect to upon cancelation

    Returns:
        json, int: JSON containing session id and status code
    """
    try:
        checkout_session = stripe.checkout.Session.create(
            success_url=success_url,
            cancel_url=cancel_url,
            customer=customer_id,
            payment_method_types=["card"],
            mode="subscription",
            line_items=[{"price": current_app.config["STRIPE_PRICE_ID"], "quantity": 1}],
        )
        return jsonify({"session_id": checkout_session["id"]}), SUCCESS
    except Exception as e:
        return jsonify({"error": {"message": str(e)}}), BAD_REQUEST


def get_billing_portal_url(customer_id: str, return_url: str) -> Tuple[str, int]:
    """
    Returns the url to a customer's billing portal.

    Args:
        customer_id (str): the stripe id of the user
        return_url (str): the url to redirect to upon leaving the billing portal

    Returns:
        json, int: JSON containing billing url and status code
    """
    try:
        billing_session = stripe.billing_portal.Session.create(
            customer=customer_id, return_url=return_url
        )
        return jsonify({"url": billing_session.url}), SUCCESS
    except Exception as e:
        return jsonify({"error": {"message": str(e)}}), BAD_REQUEST


def get_customer_status(customer_id: str) -> bool:
    """
    Returns true if a customer has access to the product (is on a trial/paying)

    Args:
        customer_id (str): the stripe id of the user

    Returns:
        has_access (boolean): true if either subscribed or trialed, false if not
    """
    has_access = False
    try:
        client_info = stripe.Customer.retrieve(customer_id, expand=["subscriptions"])
        subscriptions = client_info["subscriptions"]
        if len(subscriptions["data"]) > 0:
            curr_subscription = subscriptions["data"][0]
            status = curr_subscription["status"]
            if status in ("active", "trialing"):
                has_access = True
        return has_access
    except Exception:
        return False
