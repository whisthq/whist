import stripe

from flask import Blueprint, jsonify, request
from flask_jwt_extended import jwt_required

from app import fractalPreProcess
from app.constants.config import ENDPOINT_SECRET
from app.constants.http_codes import FORBIDDEN, NOT_ACCEPTABLE

# from app.helpers.blueprint_helpers.payment.stripe_post import (
# )
from app.helpers.utils.general.auth import fractalAuth

stripe_bp = Blueprint("stripe_bp", __name__)

# TODO consider remembering cards or not (adding source)
# currently we are not doing it


@stripe_bp.route("/stripe/<action>", methods=["POST"])
@fractalPreProcess
@jwt_required
@fractalAuth
def payment(action, **kwargs):
    body = kwargs["body"]

    # adds or cancels (deletes) a subscription to the customer
    if action == "charge":
        pass
    elif action == "cancel":
        pass
    # adds and removes products TODO
    elif action == "addProduct":
        pass  # TODO
    elif action == "removeProduct":
        pass  # TODO
    # Retrieves the stripe subscription of the customer
    elif action == "retrieve":
        pass
    elif action == "update":
        # When a customer requests to change their plan type
        pass
    return {"status": FORBIDDEN}
