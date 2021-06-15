"""Tests for stripe endpoints."""

import os

import stripe

from app.constants.http_codes import SUCCESS, BAD_REQUEST, UNAUTHORIZED
from tests.patches import function, Object

"""Here are the tests for the /stripe/customer_portal endpoint.
"""


def test_create_billing_portal_authorized(client, make_user, monkeypatch):
    """Test the /stripe/customer_portal endpoint"""

    user = make_user()

    client.login(user)

    response = client.post(
        "/stripe/customer_portal",
        json=dict(),
    )
    assert response.status_code == BAD_REQUEST

    session = Object()
    session_url = f"http://localhost/{os.urandom(8).hex()}"

    monkeypatch.setattr(session, "url", session_url)
    monkeypatch.setattr(stripe.billing_portal.Session, "create", function(returns=session))

    response = client.post(
        "/stripe/customer_portal",
        json=dict(return_url="https://fractal.co"),
    )
    print(response.json)
    assert response.status_code == SUCCESS
    assert response.json["url"] == session_url


def test_create_billing_portal_unauthorized(client):
    """Test the /stripe/customer_portal endpoint"""
    response = client.post(
        "/stripe/customer_portal",
        json=dict(return_url="https://fractal.co"),
    )
    assert response.status_code == UNAUTHORIZED
