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
from app.helpers.utils.payment.stripe_client import (
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
        json, int: Json containing session id and status code
    """
    client = StripeClient(current_app.config["STRIPE_SECRET"])
    try:
        checkout_id = client.create_checkout_session(success_url, cancel_url, customer_id, price_id)
        return (
            jsonify({"sessionId": checkout_id}),
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
        json, int: Json containing billing url and status code
    """
    client = StripeClient(current_app.config["STRIPE_SECRET"])
    try:
        url = client.create_billing_session(customer_id, return_url)
        return jsonify({"url": url}), SUCCESS
    except Exception as e:
        return jsonify({"error": {"message": str(e)}}), BAD_REQUEST


def retrieveHelper(email: str) -> Tuple[str, int]:
    client = StripeClient(current_app.config["STRIPE_SECRET"])

    try:
        info = client.get_customer_info(email)
        info["status"] = SUCCESS
        return jsonify(info), SUCCESS
    except NonexistentUser:
        return jsonify({"status": FORBIDDEN}), FORBIDDEN
    # if info is None then we will raise a typerror
    except TypeError:
        return (
            jsonify(
                {
                    "subscription": None,
                    "source": None,
                    "account_locked": False,
                    "customer": None,
                }
            ),
            PAYMENT_REQUIRED,
        )
    except Exception as e:
        fractal_logger.error("", exc_info=True)
        return jsonify({"status": INTERNAL_SERVER_ERROR}), INTERNAL_SERVER_ERROR


def hasAccess(stripe_id: str) -> bool:
    client = StripeClient(current_app.config["STRIPE_SECRET"])

    if client.is_paying or client.is_trialed:
        return True
    return False
