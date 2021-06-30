"""Tests for stripe endpoints."""

import os
from http import HTTPStatus

import pytest
import stripe

from tests.patches import function, Object

"""Here are the tests for the /stripe/customer_portal endpoint.
"""


@pytest.mark.usefixtures("authorized")
def test_create_billing_portal_authorized(client, monkeypatch):
    """Test the /stripe/customer_portal endpoint"""

    session = Object()
    session_url = f"http://localhost/{os.urandom(8).hex()}"
    monkeypatch.setattr(session, "url", session_url)
    monkeypatch.setattr(stripe.billing_portal.Session, "create", function(returns=session))
    monkeypatch.setattr("payments.get_stripe_customer_id", function(returns="cus_test"))

    response = client.post(
        "/stripe/customer_portal",
        json=dict(),
    )
    assert response.status_code == HTTPStatus.BAD_REQUEST

    response = client.post(
        "/stripe/customer_portal",
        json=dict(return_url="https://fractal.co"),
    )
    print(response.json)
    assert response.status_code == HTTPStatus.OK
    assert response.json["url"] == session_url


def test_create_billing_portal_unauthorized(client):
    """Test the /stripe/customer_portal endpoint"""
    response = client.post(
        "/stripe/customer_portal",
        json=dict(return_url="https://fractal.co"),
    )
    assert response.status_code == HTTPStatus.UNAUTHORIZED
