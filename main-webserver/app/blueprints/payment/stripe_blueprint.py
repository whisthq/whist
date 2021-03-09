from typing import Any, Dict

from flask import Blueprint, jsonify
from flask_jwt_extended import jwt_required
from sqlalchemy.orm.exc import NoResultFound

from app import fractal_pre_process
from app.constants.http_codes import FORBIDDEN
from app.helpers.blueprint_helpers.payment.stripe_post import (
    addSubscriptionHelper,
    deleteSubscriptionHelper,
    addCardHelper,
    deleteCardHelper,
    retrieveHelper,
)
from app.helpers.blueprint_helpers.payment.webhook import stripe_webhook
from app.helpers.utils.general.auth import fractal_auth
from app.helpers.utils.general.limiter import limiter, RATE_LIMIT_PER_MINUTE
from app.models import db, User

stripe_bp = Blueprint("stripe_bp", __name__, url_prefix="/stripe")


def handle_subscription_update(event: Dict[str, Any]) -> None:
    """Update a user's cached subscription status in response to a Stripe subscription event.

    In order to determine whether or not a Fractal user should receive service, we cache the
    activity status of a user's subscription in our database. The cached value is updated each time
    we receive a subscription event from Stripe's servers.

    This event handler determines how it should update the cached subscription status by reading
    the subscription event's status key.

    https://stripe.com/docs/api/subscriptions/object#subscription_object-status

    Args:
        event: A dictionary representing a Stripe Event object.
            https://stripe.com/docs/api/events/object

    Returns:
        None
    """

    subscription = event["data"]["object"]

    try:
        user = User.query.filter_by(stripe_customer_id=subscription["customer"]).one()
    except NoResultFound:
        pass
    else:
        if subscription["status"] in ("active", "trialing"):
            user.subscribed = True
        else:
            user.subscribed = False

        db.session.commit()


stripe_bp.add_url_rule(
    "/webhook",
    "webhook",
    stripe_webhook(
        {
            "customer.subscription.created": handle_subscription_update,
            "customer.subscription.deleted": handle_subscription_update,
            "customer.subscription.updated": handle_subscription_update,
        }
    ),
    methods=("POST",),
)


@stripe_bp.route("/<action>", methods=["POST"])
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
