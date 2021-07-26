from http import HTTPStatus

from flask import Blueprint
from flask_jwt_extended import jwt_required

from app import fractal_pre_process
from app.helpers.utils.general.limiter import limiter, RATE_LIMIT_PER_MINUTE
from payments import get_stripe_customer_id
from payments.stripe_helpers import (
    get_billing_portal_url,
)

stripe_bp = Blueprint("stripe_bp", __name__)


@stripe_bp.route("/stripe/customer_portal", methods=["POST"])
@limiter.limit(RATE_LIMIT_PER_MINUTE)
@fractal_pre_process
@jwt_required()
def customer_portal(**kwargs):
    """
    Returns billing portal url.

    Args:
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
        customer_id = get_stripe_customer_id()
        return_url = body["return_url"]
    except:
        return {
            "error": "The request body is incorrectly formatted, or the user is not authorized."
        }, HTTPStatus.BAD_REQUEST

    return get_billing_portal_url(customer_id, return_url)
