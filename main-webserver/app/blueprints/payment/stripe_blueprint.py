from app import *
from app.helpers.blueprint_helpers.payment.stripe_post import *

stripe_bp = Blueprint("stripe_bp", __name__)


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

    elif action == "discount":
        return discountHelper(body["code"])

    # Inserts a customer to the table
    elif action == "insert":
        return insertCustomerHelper(body["username"], body["location"])

    elif action == "update":
        # When a customer requests to change their plan type
        return updateHelper(body["username"], body["plan"])

    elif action == "referral":
        return referralHelper(body["code"], body["username"])


@stripe_bp.route("/stripe/hooks", methods=["POST"])
def hooks(**kwargs):
    body = request.get_json()

    # Endpoint for stripe webhooks
    sigHeader = request.headers["Stripe-Signature"]
    endpointSecret = os.getenv("ENDPOINT_SECRET")
    event = None

    try:
        event = stripe.Webhook.construct_event(body, sigHeader, endpointSecret)
    except ValueError as e:
        # Invalid payload
        return jsonify({"status": "Invalid payload"}), NOT_ACCEPTABLE
    except stripe.error.SignatureVerificationError as e:
        # Invalid signature
        print(e)
        return jsonify({"status": "Invalid signature"}), FORBIDDEN

    return webhookHelper(event)
