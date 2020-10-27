import stripe

from flask import Blueprint, jsonify, request
from flask_jwt_extended import jwt_required

from app import fractalPreProcess
from app.constants.config import ENDPOINT_SECRET
from app.constants.http_codes import FORBIDDEN, NOT_ACCEPTABLE
from app.helpers.blueprint_helpers.payment.stripe_post import (
    addCardHelper,
    addProductHelper,
    cancelStripeHelper,
    chargeHelper,
    deleteCardHelper,
    discountHelper,
    referralHelper,
    removeProductHelper,
    retrieveStripeHelper,
    updateHelper,
    webhookHelper,
)
from app.helpers.utils.general.auth import fractalAuth

stripe_bp = Blueprint("stripe_bp", __name__)


@stripe_bp.route("/stripe/addCard", methods=["POST"])
@fractalPreProcess
@jwt_required
def addCard(**kwargs):
    body = kwargs["body"]
    return addCardHelper(body["custId"], body["sourceId"])


@stripe_bp.route("/stripe/deleteCard", methods=["POST"])
@fractalPreProcess
@jwt_required
def deleteCard(**kwargs):
    body = kwargs["body"]

    return deleteCardHelper(body["custId"], body["cardId"])


@stripe_bp.route("/stripe/discount", methods=["POST"])
@fractalPreProcess
@jwt_required
def discount(**kwargs):
    body = kwargs["body"]
    return discountHelper(body["code"])


@stripe_bp.route("/stripe/<action>", methods=["POST"])
@fractalPreProcess
@jwt_required
@fractalAuth
def payment(action, **kwargs):
    body = kwargs["body"]

    # Adds a subscription to the customer
    if action == "charge":
        return chargeHelper(body["token"], body["username"], body["code"], body["plan"])

    elif action == "addProduct":
        return addProductHelper(body["username"], body["product"])

    elif action == "removeProduct":
        return removeProductHelper(body["username"], body["product"])

    # Retrieves the stripe subscription of the customer
    elif action == "retrieve":
        return retrieveStripeHelper(body["username"])

    # Cancel a stripe subscription
    elif action == "cancel":
        return cancelStripeHelper(body["username"])

    elif action == "update":
        # When a customer requests to change their plan type
        return updateHelper(body["username"], body["plan"])

    elif action == "referral":
        return referralHelper(body["code"], body["username"])


@stripe_bp.route("/stripe/hooks", methods=["POST"])
def hooks(**kwargs):
    body = request.get_data()

    # Endpoint for stripe webhooks
    sigHeader = request.headers["Stripe-Signature"]
    endpointSecret = ENDPOINT_SECRET
    event = None

    try:
        event = stripe.Webhook.construct_event(body, sigHeader, endpointSecret)
    except ValueError:
        # Invalid payload
        return jsonify({"status": "Invalid payload"}), NOT_ACCEPTABLE
    except stripe.error.SignatureVerificationError:
        # Invalid signature
        return jsonify({"status": "Invalid signature"}), FORBIDDEN

    return webhookHelper(event)
