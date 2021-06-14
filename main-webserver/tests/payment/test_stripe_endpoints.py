"""Tests for stripe endpoints."""

from datetime import datetime
from dateutil.relativedelta import relativedelta

import pytest
import stripe

from app.helpers.utils.general.time import date_to_unix
from app.constants.http_codes import SUCCESS, BAD_REQUEST, UNAUTHORIZED

"""Here are the tests for the /stripe/customer_portal endpoint.
"""


@pytest.mark.usefixtures("authorized")
def test_create_billing_portal_authorized(client):
    """Test the /stripe/customer_portal endpoint"""
    response = client.post(
        "/stripe/customer_portal",
        json=dict(),
    )
    assert response.status_code == BAD_REQUEST

    response = client.post(
        "/stripe/customer_portal",
        json=dict(return_url="https://fractal.co"),
    )
    assert response.status_code == SUCCESS
    assert response.json["url"]


def test_create_billing_portal_unauthorized(client):
    """Test the /stripe/customer_portal endpoint"""
    response = client.post(
        "/stripe/customer_portal",
        json=dict(return_url="https://fractal.co"),
    )
    assert response.status_code == UNAUTHORIZED
