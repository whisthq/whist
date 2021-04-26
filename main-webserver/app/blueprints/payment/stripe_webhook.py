import json
import stripe

stripe.api_key = "sk_test_51IigLsL2k8k1yyGOHRL6meRZrA2S9SKc06p27x9ltjm1UQ09JSGGzjzu25vsIBiqLXbgOjhlRAlZ8gAG2iwaWGoI00xlRGhNWw"

from flask import Flask, Blueprint, jsonify, request

stripe_webhook_bp = Blueprint("stripe_webhook_bp", __name__)


@limiter.limit(RATE_LIMIT_PER_MINUTE)  # honestly don't know if this should be here
@fractal_pre_process
@jwt_required()
@fractal_auth
@stripe_webhook_bp.route("/stripe_webhook", methods=["POST"])
def webhook(**kwargs):
    event = None
    payload = request.data
    try:
        event = json.loads(payload)
    except:
        print("Webhook error while parsing basic request.")
        return jsonify(success=False)

    if event["type"] == "payment_intent.succeeded":
        return {"message": event["id"]}
