# import stripe
# from app.helpers.utils.general.logs import fractal_log

from flask import Blueprint, jsonify  # , request
from flask_jwt_extended import jwt_required

from app import fractal_pre_process

# from app.constants.config import ENDPOINT_SECRET
from app.constants.http_codes import FORBIDDEN  # , NOT_ACCEPTABLE

from app.helpers.blueprint_helpers.payment.stripe_post import (
    addSubscriptionHelper,
    deleteSubscriptionHelper,
    addCardHelper,
    deleteCardHelper,
    retrieveHelper,
)
from app.helpers.utils.general.auth import fractal_auth

stripe_bp = Blueprint("stripe_bp", __name__)


@stripe_bp.route("/stripe/<action>", methods=["POST"])
@fractal_pre_process
@jwt_required
@fractal_auth
def payment(action, **kwargs):
    """Covers payment endpoints for stripe. Right now has seven endpoints:

        addSubscription - (token, email, plan, code?) -> Will add or modify a subscription
            depending on whether a user exists.

        modifySubscription - (token, email) -> Carbon copy of addSubscription. Meant to make
            naming easier when the user already has a subscription and you want to modify it.

        deleteSubscription - (email) -> Delete a subscription, but does not delete a customer.

        addCard - Adds a card. Stripe does not check whether the card already exists (at least,
            it doesn't check if the #s are equal; it might check if ALL the fields are though...
            not sure).

        modifyCard - (token, email) -> Carbon copy of addCard.

        deleteCard - (email) -> Deletes a card matching by last four digits.

        retrieve - (email) -> Returns a formatted json object with information about
            a customer. Check stripe_client.get_customer_info to see the format returned.

    When we say we require (token, email, ...) etc... the expected format from the site or client
    app is having the body of the post request be like {
        "token" : "this is a token string",
        "email" : "this is an email string"
        ...
    } where the keyword 'token' or 'email' or 'code' etc are the string keys and the string body
    is the payload expected. Types are as such:

        token: stripe token id string (has format tok_andabunchofchars)

        email: regular email string

        plan: string for the plan we want, should match the name (not the id) of the plan in stripe
            the format is = "Fractal Monthly" | "Fractal Unlimited" | "Fractal Hourly"

        code: a code that someone has for referrals, check the config db for the format (it's a
            constant length of random chars I think.)


    Return values vary based on the helpers. Check in  app/helpers/blueprint_helpers/payment_stripe_post
    for details on how the returns function. Generally speaking, 500 will mean something went wrong,
    200 will mean everything went swimmingly, 403 (forbidden) will mean that the user did not exist or
    something similar, 400 will mean that necessary information was not given and the request could
    not be processed (or that the user has the permissions to do the action, but not the context: for
    example a user who has not yet signed up for a subscription and has no stripe customer id cannot
    make stripe actions even if they are a full user).
    """
    body = kwargs["body"]

    # these add a subscription or remove (or modify)
    if action == "addSubscription" or action == "modifySubscription":
        return addSubscriptionHelper(body["email"], body["plan"])
    elif action == "deleteSubscription":
        return deleteSubscriptionHelper(body["email"])
    # these will add a card as a source or remove (or modify) for future payment
    elif action == "addCard" or action == "modifyCard":
        return addCardHelper(body["email"], body["source"])
    elif action == "deleteCard":
        return deleteCardHelper(body["email"], body["source"])
    # Retrieves the stripe subscription of the customer so we can tell them some basic info
    elif action == "retrieve":
        return retrieveHelper(body["email"])
    return jsonify({"status": FORBIDDEN}), FORBIDDEN


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
