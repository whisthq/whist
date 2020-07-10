from app import *
from app.helpers.blueprint_helpers.stripe_post import *

stripe_bp = Blueprint("stripe_bp", __name__)


@stripe_bp.route("/stripe/<action>", methods=["POST"])
@fractalPreProcess
@fractalAuth
def payment(action, **kwargs):
    body = kwargs["body"]

    # Adds a subscription to the customer
    if action == "charge":
        body = request.get_json()
        return chargeHelper(body["token"], body["email"], body["code"], body["plan"])

    # Retrieves the stripe subscription of the customer
    elif action == "retrieve":
        return retrieveStripeHelper(body["email"])

    # Cancel a stripe subscription
    elif action == "cancel":
        return cancelStripeHelper(body["email"])

    elif action == "discount":
        return discountHelper(body["code"])

    # Inserts a customer to the table
    elif action == "insert":
        return insertCustomerHelper(body["email"], body["location"])

    # Endpoint for stripe webhooks
    elif action == "hooks":
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
            return jsonify({"status": "Invalid signature"}), FORBIDDEN

        return webhookHelper(event)

    elif action == "update":
        # When a customer requests to change their plan type
        return updateHelper(body["username"], body["plan"])

    elif action == "referral":
        return referralHelper(body["code"], body["username"])
