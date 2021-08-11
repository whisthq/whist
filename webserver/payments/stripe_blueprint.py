from typing import Any

import stripe
from flask import Blueprint, current_app, jsonify
from flask_jwt_extended import jwt_required

from payments import get_stripe_customer_id, get_subscription_status

stripe_bp = Blueprint("stripe_bp", __name__)


@stripe_bp.route("/payment_portal_url")
@jwt_required()
def payment_portal() -> Any:
    """Creates a Stripe checkout or customer portal session an returns the URL.

    If the authenticated user already has an active Fractal subscription, this endpoint creates a
    Stripe customer portal session and returns its URL. By navigating to the URL, the user can use
    a Stripe-hosted form to manage their subscription and billing information.

    If the authenticated user does not currently have an active Fractal subscription, either
    because they have never signed up for a Fractal plan before or because their subscription has
    been automatically (due to a lack of payment) or manually cancelled, this endpoint creates a
    Stripe checkout session and returns its URL. By navigating to the URL, the user can use a
    Stripe-hosted form to sign up for a new Fractal subscription.

    Returns:
        A JSON object containing a single key "url", whose value is the URL of a Stripe checkout
        or customer portal session.
    """

    customer_id = get_stripe_customer_id()

    # If the Stripe customer ID is not present in the access token, something has gone horribly
    # wrong.
    assert customer_id

    if get_subscription_status() in (None, "canceled", "incomplete_expired", "unpaid"):
        # Any user subscriptions that the user might have are in the cancelled state and cannot be
        # renewed. The user must sign up for a new subscription.
        session = stripe.checkout.Session.create(
            customer=customer_id,
            line_items=[
                {
                    "price": current_app.config["STRIPE_PRICE_ID"],
                    "quantity": 1,
                }
            ],
            payment_method_types=("card",),
            success_url="http://localhost/callback/payment?success=true",
            cancel_url="http://localhost/callback/payment?success=false",
            mode="subscription",
        )
    else:
        # The user has a non-cancelled subscription.
        session = stripe.billing_portal.Session.create(
            customer=customer_id,
            return_url="http://localhost/callback/payment",
        )

    return jsonify(url=session.url)
