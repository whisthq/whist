"""Contains a function that generates Stripe webhook endpoint Flask view functions.

Example:

    app = Flask(__name__)
    webhook = stripe_webhook({"charge.succeeded": ...})

    app.config.update(STRIPE_WEBHOOK_SECRET="whsec_...")
    app.register_url_route("/stripe/events", "stripe_webhook", webhook, methods=("POST",))

"""

import json

from typing import Any, Callable, Dict

import stripe

from flask import abort, current_app, request, Response


def stripe_webhook(
    handlers: Dict[str, Callable[[Dict[str, Any]], None]],
    secret_lookup_key: str = "STRIPE_WEBHOOK_SECRET",
) -> Callable[[], Response]:
    """Construct a Stripe webhook view function.

    Args:
        handlers: A dictionary mapping Stripe event strings to handler functions. Each handler
            should be a void function that takes as input a single dictionary representing a Stripe
            event.
        secret_lookup_key: The key in the Flask application's configuration whose value is this
            webhook's endpoint secret.

    Returns:
        A Flask view function that takes no arguments. It should return an instance of the Flask
        Response object.
    """

    def _webhook():
        # Verify the request payload's signature and parse the payload into a Stripe event
        # dictionary.
        try:
            event = stripe.Webhook.construct_event(
                request.data,
                request.headers.get("Stripe-Signature"),
                current_app.config[secret_lookup_key],
            )
        except json.JSONDecodeError:
            abort(400)
        except stripe.error.SignatureVerificationError:
            abort(401)

        # Handle the event if a handler has been supplied.
        try:
            handler = handlers[event["type"]]
        except KeyError:
            pass
        else:
            handler(event)

        return "OK"

    return _webhook
