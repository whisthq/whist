import stripe

from flask import Blueprint, jsonify, request
from flask_jwt_extended import jwt_required

from app import fractalPreProcess
from app.constants.config import ENDPOINT_SECRET
from app.constants.http_codes import FORBIDDEN, NOT_ACCEPTABLE

from app.helpers.blueprint_helpers.payment.stripe_post import (
    addSubscriptionHelper,
    deleteSubscriptionHelper,
    addCardHelper,
    deleteCardHelper,
    retrieveHelper,
)
from app.helpers.utils.general.auth import fractalAuth

stripe_bp = Blueprint("stripe_bp", __name__)


@stripe_bp.route("/stripe/<action>", methods=["POST"])
@fractalPreProcess
@jwt_required
@fractalAuth
def payment(action, **kwargs):
    body = kwargs["body"]

    # these add a subscription or remove (or modify)
    if action == "addSubscription" or action == "modifySubscription":
        return addSubscriptionHelper(body["email"], body["token"], body["plan"], body["code"])
    elif action == "deleteSubscription":
        return deleteSubscriptionHelper(body["email"])
    # these will add a card as a source or remove (or modify) for future payment
    elif action == "addCard" or action == "modifyCard":
        return addCardHelper(body["email"], body["token"])
    elif action == "deleteCard":
        return deleteCardHelper(body["email"])
    # Retrieves the stripe subscription of the customer so we can tell them some basic info
    elif action == "retrieve":
        return retrieveHelper(body["email"])
    return {"status": FORBIDDEN}


## TODO
# in the future we may want to re-add endpoints to let them add products
# basically, there are products which they can buy multiple units of
# right now, not really so it's not implemented
# there should be some scaffolding you can use in the client to make it happen

## TODO not sure what this is used by exactly
# @stripe_bp.route("/stripe/hooks", methods=["POST"])
# def hooks(**kwargs):
#     body = request.get_data()

#     # Endpoint for stripe webhooks
#     sigHeader = request.headers["Stripe-Signature"]
#     endpointSecret = ENDPOINT_SECRET
#     event = None

#     try:
#         event = stripe.Webhook.construct_event(body, sigHeader, endpointSecret)
#     except ValueError:
#         # Invalid payload
#         return jsonify({"status": "Invalid payload"}), NOT_ACCEPTABLE
#     except stripe.error.SignatureVerificationError:
#         # Invalid signature
#         return jsonify({"status": "Invalid signature"}), FORBIDDEN

#     return webhookHelper(event)
