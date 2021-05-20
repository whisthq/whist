from flask import Blueprint, jsonify, request, current_app
from flask_jwt_extended import jwt_required

from app import fractal_pre_process

# from app.constants.config import ENDPOINT_SECRET
from app.constants.http_codes import BAD_REQUEST, SUCCESS

from payments.stripe_helpers import (
    checkout_helper,
    billing_portal_helper,
    has_access_helper,
    webhook_helper,
)
from app.helpers.utils.general.limiter import limiter, RATE_LIMIT_PER_MINUTE

stripe_bp = Blueprint("stripe_bp", __name__)

## TODO
# in the future we may want to re-add endpoints to let them add products
# basically, there are products which they can buy multiple units of
# right now, not really so it's not implemented
# there should be some scaffolding you can use in the client to make it happen


@stripe_bp.route("/stripe/can_access_product", methods=["POST"])
@limiter.limit(RATE_LIMIT_PER_MINUTE)
@fractal_pre_process
@jwt_required()
def can_access_product(**kwargs):
    """Returns a boolean with whether the given customer can access Fractal

    Args:
        customer_id (str): the stripe id of the user (different than email)

    Returns:
        json, int: JSON with whether the user has access, status code. The JSON is in the format
            {
                "subscribed": returns whether the user has access
            }
                or

            {
                "error": {
                    "message": error message
                }
            }
                if the customer does not exist
    """
    body = kwargs["body"]

    try:
        customer_id = body["customer_id"]
    except:
        return {"error": "The request body is incorrectly formatted."}, BAD_REQUEST

    # TODO: use jwt _+ Auth0 to figure out a user's stripe customer_id

    return has_access_helper(customer_id)


@stripe_bp.route("/stripe/create_checkout_session", methods=["POST"])
@limiter.limit(RATE_LIMIT_PER_MINUTE)
@fractal_pre_process
@jwt_required()
def create_checkout_session(**kwargs):
    """
    Returns checkout session id from a given product and customer

    Args:
        customer_id (str): the stripe id of the user
        success_url (str): url to redirect to upon completion success
        cancel_url (str): url to redirect to upon cancelation

    Returns:
        json, int: JSON containing session id and status code. The JSON is in the format
            {
                "session_id": the checkout session id
            }
                or

            {
                "error": {
                    "message": error message
                }
            }
                if the creation fails
    """
    body = kwargs["body"]
    try:
        customer_id = body["customer_id"]
        success_url = body["success_url"]
        cancel_url = body["cancel_url"]
    except:
        return {"error": "The request body is incorrectly formatted."}, BAD_REQUEST

    # TODO: use jwt _+ Auth0 to figure out a user's stripe customer_id

    return checkout_helper(success_url, cancel_url, customer_id)


@stripe_bp.route("/stripe/customer_portal", methods=["POST"])
@limiter.limit(RATE_LIMIT_PER_MINUTE)
@fractal_pre_process
@jwt_required()
def customer_portal(**kwargs):
    """
    Returns billing portal url.

    Args:
        customer_id (str): the stripe id of the user
        return_url (str): the url to redirect to upon leaving the billing portal

    Returns:
        json, int: JSON containing billing url and status code. The JSON is in the format
            {
                "url": the billing portal url
            }
                or

            {
                "error": {
                    "message": error message
                }
            }
                if the creation fails
    """

    body = kwargs["body"]
    try:
        customer_id = body["customer_id"]
        return_url = body["return_url"]
    except:
        return {"error": "The request body is incorrectly formatted."}, BAD_REQUEST

    # TODO: use jwt _+ Auth0 to figure out a user's stripe customer_id

    return billing_portal_helper(customer_id, return_url)


@fractal_pre_process
@stripe_bp.route("/stripe/webhook", methods=["POST"])
def webhook():
    """
    Endpoint for receiving all stripe webhook calls
    """
    event = None
    try:
        event = webhook_helper(request)
    except Exception as e:
        # ValueError: Invalid payload
        # stripe.error.SignatureVerificationError: Invalid signature
        return jsonify({"error": {"message": str(e)}}), BAD_REQUEST

    # TODO: uncomment this once the webhook is in the stripe dashboard and STRIPE_WEBHOOK_SECRET has been populated
    # if event.type == "invoice.created":
    #     print("invoice created")
    # elif event.type == "invoice.payment_failed":
    #     print("payment failed")
    # else:
    #     print("Unhandled event type {}".format(event.type))

    # TODO: delete this once the webhook is in the stripe dashboard and STRIPE_WEBHOOK_SECRET has been populated
    if event["type"] == "invoice.created":
        print("invoice created")
    elif event["type"] == "invoice.payment_failed":
        print("payment failed")
    else:
        print("Unhandled event type {}".format(event.type))

    return "OK", SUCCESS
