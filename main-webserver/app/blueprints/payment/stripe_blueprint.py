from flask import Blueprint, jsonify  # , request
from flask_jwt_extended import jwt_required

from app import fractal_pre_process

# from app.constants.config import ENDPOINT_SECRET
from app.constants.http_codes import FORBIDDEN  # , NOT_ACCEPTABLE

from app.helpers.blueprint_helpers.payment.stripe_post import (
    retrieveHelper,
)
from app.helpers.utils.general.auth import fractal_auth
from app.helpers.utils.general.limiter import limiter, RATE_LIMIT_PER_MINUTE

import stripe


stripe_bp = Blueprint("stripe_bp", __name__)


@stripe_bp.route("/stripe/<action>", methods=["POST"])
@limiter.limit(RATE_LIMIT_PER_MINUTE)
@fractal_pre_process
@jwt_required()
@fractal_auth
def payment(action, **kwargs):
    """Covers payment endpoints for stripe. Right now has seven endpoints:

        addSubscription - (email, plan) -> Will add or modify a subscription
            depending on whether a user exists.

        modifySubscription - (email, plan) -> Carbon copy of addSubscription. Meant to make
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
    } where the keyword 'token' or 'email' etc are the string keys and the string body
    is the payload expected. Types are as such:

        token: stripe token id string (has format tok_andabunchofchars)

        email: regular email string

        plan: string for the plan we want, should match the name (not the id) of the plan in stripe
            the format is = "Fractal Monthly" | "Fractal Unlimited" | "Fractal Hourly"

    Return values vary based on the helpers. Check in  app/helpers/blueprint_helpers/payment_stripe_post
    for details on how the returns function. Generally speaking, 500 will mean something went wrong,
    200 will mean everything went swimmingly, 403 (forbidden) will mean that the user did not exist or
    something similar, 400 will mean that necessary information was not given and the request could
    not be processed (or that the user has the permissions to do the action, but not the context: for
    example a user who has not yet signed up for a subscription and has no stripe customer id cannot
    make stripe actions even if they are a full user).
    """
    body = kwargs["body"]

    if action == "retrieve":
        return retrieveHelper(body["email"])
    return jsonify({"status": FORBIDDEN}), FORBIDDEN


## TODO
# in the future we may want to re-add endpoints to let them add products
# basically, there are products which they can buy multiple units of
# right now, not really so it's not implemented
# there should be some scaffolding you can use in the client to make it happen


@limiter.limit(RATE_LIMIT_PER_MINUTE)
@fractal_pre_process
@jwt_required()
@fractal_auth
@stripe_bp.route("/stripe/create-checkout-session", methods=["POST"])
def create_checkout_session():
    data = json.loads(request.data)
    try:
        checkout_session = stripe.checkout.Session.create(
            success_url="https://example.com/success.html?session_id={CHECKOUT_SESSION_ID}",
            cancel_url="https://example.com/canceled.html",
            payment_method_types=["card"],
            mode="subscription",
            line_items=[
                {
                    "price": data["priceId"],
                    "quantity": 1,
                }
            ],
        )
        return jsonify({"sessionId": checkout_session["id"]})
    except Exception as e:
        return jsonify({"error": {"message": str(e)}}), 400


@limiter.limit(RATE_LIMIT_PER_MINUTE)
@fractal_pre_process
@jwt_required()
@fractal_auth
@stripe_bp.route("/stripe/customer-portal", methods=["POST"])
def customer_portal():
    # This is the URL to which the customer will be redirected after they are
    # done managing their billing with the portal.
    return_url = "fractal.co"  # TODO: not sure what goes here

    session = stripe.billing_portal.Session.create(
        customer="{{CUSTOMER_ID}}", return_url=return_url
    )
    return jsonify({"url": session.url})


@fractal_pre_process
@jwt_required()
@fractal_auth
@stripe_bp.route("/stripe/webhook", methods=["POST"])
def webhook():
    event = None
    payload = request.data
    try:
        event = json.loads(payload)
    except:
        print("Webhook error while parsing basic request.")
        return jsonify(success=False)

    if event["type"] == "payment_intent.succeeded":
        return {"message": event["id"]}
