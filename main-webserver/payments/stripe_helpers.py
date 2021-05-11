from typing import Tuple
from flask import current_app, jsonify

from functools import reduce

from app.constants.http_codes import (
    BAD_REQUEST,
    NOT_ACCEPTABLE,
    PAYMENT_REQUIRED,
    SUCCESS,
    FORBIDDEN,
    INTERNAL_SERVER_ERROR,
)
from app.helpers.utils.general.logs import fractal_logger
from payments.stripe_client import (
    StripeClient,
    NonexistentUser,
    NonexistentStripeCustomer,
    RegionNotSupported,
    InvalidStripeToken,
    InvalidOperation,
)


def checkout_helper(
    success_url: str, cancel_url: str, customer_id: str, price_id: str
) -> Tuple[str, int]:
    """
    Returns checkout session id from Stripe client

    Args:
        customer_id (str): the stripe id of the user
        price_id (str): the price id of the product (subscription)
        success_url (str): url to redirect to upon completion success
        cancel_url (str): url to redirect to upon cancelation

    Returns:
        json, int: JSON containing session id and status code
    """
    client = StripeClient(current_app.config["STRIPE_SECRET"])
    try:
        checkout_id = client.create_checkout_session(success_url, cancel_url, customer_id, price_id)
        return (
            jsonify({"session_id": checkout_id}),
            SUCCESS,
        )
    except Exception as e:
        return jsonify({"error": {"message": str(e)}}), BAD_REQUEST


def billing_portal_helper(customer_id: str, return_url: str) -> Tuple[str, int]:
    """
    Returns billing portal url.

    Args:
        customer_id (str): the stripe id of the user
        return_url (str): the url to redirect to upon leaving the billing portal

    Returns:
        json, int: JSON containing billing url and status code
    """
    client = StripeClient(current_app.config["STRIPE_SECRET"])
    try:
        url = client.create_billing_session(customer_id, return_url)
        return jsonify({"url": url}), SUCCESS
    except Exception as e:
        return jsonify({"error": {"message": str(e)}}), BAD_REQUEST


def has_access_helper(stripe_id: str) -> bool:
    """
    Returns whether customer is subscribed or trialed

    Args:
        customer_id (str): the stripe id of the user

    Returns:
        json, int: JSON containing true or false if either subscribed or trialed and status code
    """
    client = StripeClient(current_app.config["STRIPE_SECRET"])
    try:
        subscribed = client.is_paying(stripe_id) or client.is_trialed(stripe_id)
        return jsonify(subscribed=subscribed), SUCCESS
    except Exception as e:
        return jsonify({"error": {"message": str(e)}}), BAD_REQUEST
