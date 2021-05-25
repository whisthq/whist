"""Stripe customer portal billing session endpoint.

https://stripe.com/docs/billing/subscriptions/customer-portal
"""

import stripe

from flask import Blueprint, jsonify
from flask_jwt_extended import get_jwt, jwt_required

stripe_bp = Blueprint("stripe", __name__)


@stripe_bp.route("/customer-portal", methods=("POST",))
@jwt_required()
def customer_portal():
    """Create a Stripe customer billing portal session.

    Stripe customers can use billing portal sessions to manage their subscriptions and payment
    information using Stripe-hosted forms that can Fractal developers can configure using the
    Stripe dashboard.

    This implementation is inspired by
    https://stripe.com/docs/billing/subscriptions/checkout#customer-portal.

    Returns:
        A dictionary containing a single key "url", whose value is the URL that clients may use to
        access the customer portal session.
    """

    customer_id = get_jwt()["https://api.fractal.co/stripe_customer_id"]
    session = stripe.billing_portal.Session.create(
        customer=customer_id,
        return_url="http://localhost/portal-close",
    )

    return jsonify(url=session.url)
