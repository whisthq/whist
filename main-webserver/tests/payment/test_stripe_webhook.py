"""TODO"""

import pytest
import stripe

from werkzeug.exceptions import BadRequest, Unauthorized

from app.helpers.blueprint_helpers.payment.webhook import stripe_webhook
from tests.patches import function


def test_bad_payload(app, monkeypatch):
    """Raise BadRequest upon receipt of invalid JSON."""

    # Bypass signature verification.
    monkeypatch.setattr(stripe.WebhookSignature, "verify_header", function())

    webhook = stripe_webhook({})

    # The empty string cannot be parsed by json.loads()
    with app.test_request_context("/", data=""):
        with pytest.raises(BadRequest):
            webhook()


def test_bad_signature(app):
    """Raise Unauthorized upon signature verification failure."""

    webhook = stripe_webhook({})

    with app.test_request_context("/"):
        with pytest.raises(Unauthorized):
            webhook()


def test_bad_event(app, monkeypatch):
    """Return successfully upon receipt of an event with an unrecognized type."""

    # Bypass signature verification.
    monkeypatch.setattr(stripe.WebhookSignature, "verify_header", function())

    webhook = stripe_webhook({})

    with app.test_request_context("/", json={"type": "test.event"}):
        assert webhook() == "OK"


def test_webhook(app, monkeypatch):
    """Process an event with a recognized type successfully."""

    handled = []
    test_event = {"type": "test.event"}

    # Bypass signature verification.
    monkeypatch.setattr(stripe.WebhookSignature, "verify_header", function())

    def handler(event):
        handled.append(event)
        assert event == test_event

    # Create a view function that handles test events.
    webhook = stripe_webhook({test_event["type"]: handler})

    with app.test_request_context("/", json=test_event):
        assert webhook() == "OK"

    # Make sure the event handler actually got called.
    assert handled
