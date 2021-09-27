"""Tests for the access control helpers check_payment() and @payment_required"""

import os
from http import HTTPStatus

import pytest
from flask import current_app, Flask
from flask_jwt_extended import create_access_token, JWTManager, verify_jwt_in_request

from app.helpers.utils.general.limiter import limiter
from payments import (
    check_payment,
    get_customer_id,
    get_subscription_status,
    payment_required,
    PaymentRequired,
)
from tests.client import FractalAPITestClient
from tests.patches import function


@pytest.fixture
def app():
    """Create a test Flask application.

    Returns:
        A Flask application instance.
    """

    _app = Flask(__name__)
    _app.test_client_class = FractalAPITestClient
    _app.config["JWT_SECRET_KEY"] = "secret"
    _app.config["STRIPE_CUSTOMER_ID_CLAIM"] = "https://api.fractal.co/stripe_customer_id"
    _app.config["STRIPE_SUBSCRIPTION_STATUS_CLAIM"] = "https://api.fractal.co/subscription_status"
    _app.config["ENVIRONMENT"] = "TESTING"

    @_app.errorhandler(PaymentRequired)
    def _handle_payment_required(_error):
        return HTTPStatus.PAYMENT_REQUIRED.phrase, HTTPStatus.PAYMENT_REQUIRED

    JWTManager(_app)
    limiter.init_app(_app)

    return _app


def test_get_missing_customer_id():
    """Ensure that get_customer_id() returns None when the access token contains no customer ID."""

    token = create_access_token("test")

    with current_app.test_request_context(headers={"Authorization": f"Bearer {token}"}):
        verify_jwt_in_request()
        assert get_customer_id() is None


def test_get_missing_subscription_status():
    """Return None from get_subscription_status() if the subscription status claim is omitted."""

    token = create_access_token("test")

    with current_app.test_request_context(headers={"Authorization": f"Bearer {token}"}):
        verify_jwt_in_request()
        assert get_subscription_status() is None


def test_get_customer_id():
    """Ensure that get_customer_id() returns the customer ID claimed in the access token."""

    customer_id = f"cus_{os.urandom(8).hex()}"
    token = create_access_token(
        "test", additional_claims={current_app.config["STRIPE_CUSTOMER_ID_CLAIM"]: customer_id}
    )

    with current_app.test_request_context(headers={"Authorization": f"Bearer {token}"}):
        verify_jwt_in_request()
        assert get_customer_id() == customer_id


@pytest.mark.parametrize(
    "subscription_status",
    ("active", "past_due", "unpaid", "canceled", "incomplete", "incomplete_expired", "trialing"),
)
def test_get_subscription_status(subscription_status):
    """Ensure that get_subscription_status() extracts the subscription status correctly."""

    token = create_access_token(
        "test",
        additional_claims={
            current_app.config["STRIPE_SUBSCRIPTION_STATUS_CLAIM"]: subscription_status
        },
    )

    with current_app.test_request_context(headers={"Authorization": f"Bearer {token}"}):
        verify_jwt_in_request()
        assert get_subscription_status() == subscription_status


@pytest.mark.parametrize(
    "subscription_status", ("past_due", "unpaid", "canceled", "incomplete", "incomplete_expired")
)
def test_check_payment_invalid(monkeypatch, subscription_status):
    """Ensure that check_payment() raises PaymentRequired when there is no valid subscription.

    Subscriptions are considered valid if and only if they are in the "active" or "trialing"
    states.
    """

    monkeypatch.setattr("payments.get_subscription_status", function(returns=subscription_status))

    with current_app.test_request_context():
        with pytest.raises(PaymentRequired):
            check_payment()


@pytest.mark.parametrize("subscription_status", ("active", "trialing"))
def test_check_payment_valid(monkeypatch, subscription_status):
    """Ensure that check_payment() returns when there is a valid subscription."""

    monkeypatch.setattr("payments.get_subscription_status", function(returns=subscription_status))

    with current_app.test_request_context():
        check_payment()


@pytest.mark.parametrize(
    "mock_kwargs, status_code",
    [
        [{}, HTTPStatus.OK],
        [{"raises": PaymentRequired}, HTTPStatus.PAYMENT_REQUIRED],
    ],
)
def test_payment_required(client, make_user, mock_kwargs, monkeypatch, status_code):
    """Ensure that the @payment_required decorator returns correct HTTP response codes.

    @payment_required should cause the application to return 402 when check_payment() raises
    PaymentRequired. Otherwise, it should not modify the view function's return value.
    """

    @current_app.route("/")
    @payment_required
    def index():
        return HTTPStatus.OK.phrase, HTTPStatus.OK

    user = make_user()

    client.login(user)
    monkeypatch.setattr("payments.check_payment", function(**mock_kwargs))

    assert client.get("/").status_code == status_code


@pytest.mark.parametrize(
    "login_kwargs, status_code",
    [
        [{}, HTTPStatus.PAYMENT_REQUIRED],
        [{"admin": True}, HTTPStatus.OK],
    ],
)
def test_payment_required_token(client, login_kwargs, make_user, status_code):
    """Ensure that the @payment_required interprets the contents of the access token correctly.

    Administrators (i.e. users whose access tokens contain "admin" in the "scope" claim) should
    always be permitted to access the Webserver. The application should return 402 if the Stripe
    customer ID is not present in the access token.
    """

    @current_app.route("/")
    @payment_required
    def index():
        return HTTPStatus.OK.phrase, HTTPStatus.OK

    user = make_user()

    client.login(user, **login_kwargs)

    assert client.get("/").status_code == status_code
