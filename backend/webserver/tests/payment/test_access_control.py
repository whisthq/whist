"""Tests for the access control helpers check_payment() and @payment_required"""

import http
import os
from http import HTTPStatus
from typing import Any, Callable, Dict, Tuple

import pytest
from flask import current_app, Flask
from flask_jwt_extended import create_access_token, JWTManager, verify_jwt_in_request
import stripe

from app.utils.general.limiter import limiter
from app.utils.stripe.payments import (
    check_payment,
    get_customer_id,
    get_subscription_status,
    get_stripe_subscription_status,
    payment_required,
    PaymentRequired,
)
from tests.client import WhistAPITestClient
from tests.patches import function


@pytest.fixture
def app() -> Flask:
    """Create a test Flask application.

    Returns:
        A Flask application instance.
    """

    _app = Flask(__name__)
    _app.test_client_class = WhistAPITestClient
    _app.config["JWT_SECRET_KEY"] = "secret"
    _app.config["STRIPE_CUSTOMER_ID_CLAIM"] = "https://api.fractal.co/stripe_customer_id"
    _app.config["STRIPE_SUBSCRIPTION_STATUS_CLAIM"] = "https://api.fractal.co/subscription_status"
    _app.config["MONTHLY_PRICE_IN_DOLLARS"] = 50
    _app.config["ENVIRONMENT"] = "TESTING"

    @_app.errorhandler(PaymentRequired)
    def _handle_payment_required(_error: Any) -> Tuple[str, HTTPStatus]:
        return HTTPStatus.PAYMENT_REQUIRED.phrase, HTTPStatus.PAYMENT_REQUIRED

    JWTManager(_app)
    limiter.init_app(_app)

    return _app


def test_get_missing_customer_id() -> None:
    """Ensure that get_customer_id() returns None when the access token contains no customer ID."""

    token = create_access_token("test")

    with current_app.test_request_context(headers={"Authorization": f"Bearer {token}"}):
        verify_jwt_in_request()
        assert get_customer_id() is None


def test_get_missing_subscription_status() -> None:
    """Return None from get_subscription_status() if the subscription status claim is omitted."""

    token = create_access_token("test")

    with current_app.test_request_context(headers={"Authorization": f"Bearer {token}"}):
        verify_jwt_in_request()
        assert get_subscription_status() is None


def test_get_no_subscription_status_stripe(monkeypatch: pytest.MonkeyPatch) -> None:
    """Ensure that get_stripe_subscription_status() returns None when no customer can be found."""

    monkeypatch.setattr(
        stripe.Subscription,
        "list",
        function(raises=stripe.error.InvalidRequestError("No such customer", "id")),
    )

    token = create_access_token("test")

    with current_app.test_request_context(headers={"Authorization": f"Bearer {token}"}):
        verify_jwt_in_request()
        assert get_stripe_subscription_status("DNE") is None


def test_no_active_subscriptions(monkeypatch):  # type: ignore[no-untyped-def]
    """Ensure that get_stripe_subscription_status() returns None for no active subscriptions."""

    monkeypatch.setattr(stripe.Subscription, "list", function(returns={"data": []}))

    token = create_access_token("test")

    with current_app.test_request_context(headers={"Authorization": f"Bearer {token}"}):
        verify_jwt_in_request()
        assert get_stripe_subscription_status("cus_test") is None


def test_get_customer_id() -> None:
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
def test_get_subscription_status(subscription_status: str) -> None:
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
    "subscription_status",
    ("active", "past_due", "unpaid", "canceled", "incomplete", "incomplete_expired", "trialing"),
)
def test_get_stripe_subscription_status(
    subscription_status: str, monkeypatch: pytest.MonkeyPatch
) -> None:
    """Ensure that get_subscription_status() extracts the subscription status correctly."""

    monkeypatch.setattr(
        stripe.Subscription, "list", function(returns={"data": [{"status": subscription_status}]})
    )

    customer_id = f"cus_{os.urandom(8).hex()}"
    token = create_access_token(
        "test",
        additional_claims={
            current_app.config["STRIPE_SUBSCRIPTION_STATUS_CLAIM"]: "WrongStatus",
            current_app.config["STRIPE_CUSTOMER_ID_CLAIM"]: customer_id,
        },
    )

    with current_app.test_request_context(headers={"Authorization": f"Bearer {token}"}):
        verify_jwt_in_request()
        assert get_stripe_subscription_status(customer_id) == subscription_status


@pytest.mark.parametrize(
    "subscription_status", ("past_due", "unpaid", "canceled", "incomplete", "incomplete_expired")
)
def test_check_payment_invalid(monkeypatch: pytest.MonkeyPatch, subscription_status: str) -> None:
    """Ensure that check_payment() raises PaymentRequired when there is no valid subscription.

    Subscriptions are considered valid if and only if they are in the "active" or "trialing"
    states.
    """

    monkeypatch.setattr(
        "app.utils.stripe.payments.get_subscription_status", function(returns=subscription_status)
    )

    with current_app.test_request_context():
        with pytest.raises(PaymentRequired):
            check_payment()


@pytest.mark.parametrize("subscription_status", ("active", "trialing"))
def test_check_payment_valid(monkeypatch: pytest.MonkeyPatch, subscription_status: str) -> None:
    """Ensure that check_payment() returns when there is a valid subscription."""

    monkeypatch.setattr(
        "app.utils.stripe.payments.get_subscription_status", function(returns=subscription_status)
    )

    with current_app.test_request_context():
        check_payment()


@pytest.mark.parametrize(
    "mock_kwargs, status_code",
    [[{}, HTTPStatus.OK], [{"raises": PaymentRequired}, HTTPStatus.PAYMENT_REQUIRED]],
)
def test_payment_required(
    client: WhistAPITestClient,
    make_user: Callable[..., str],
    mock_kwargs: Dict[str, PaymentRequired],
    monkeypatch: pytest.MonkeyPatch,
    status_code: HTTPStatus,
) -> None:
    """Ensure that the @payment_required decorator returns correct HTTP response codes.

    @payment_required should cause the application to return 402 when check_payment() raises
    PaymentRequired. Otherwise, it should not modify the view function's return value.
    """

    @current_app.route("/")
    @payment_required
    def index() -> Tuple[str, HTTPStatus]:
        return HTTPStatus.OK.phrase, HTTPStatus.OK

    user = make_user()

    client.login(user)
    monkeypatch.setattr("app.utils.stripe.payments.check_payment", function(**mock_kwargs))

    assert client.get("/").status_code == status_code


@pytest.mark.parametrize(
    "admin, status_code",
    [[False, HTTPStatus.PAYMENT_REQUIRED], [True, HTTPStatus.OK]],
)
def test_payment_required_token(
    client: WhistAPITestClient,
    admin: bool,
    make_user: Callable[..., str],
    status_code: HTTPStatus,
) -> None:
    """Ensure that the @payment_required interprets the contents of the access token correctly.

    Administrators (i.e. users whose access tokens contain "admin" in the "scope" claim) should
    always be permitted to access the Webserver. The application should return 402 if the Stripe
    customer ID is not present in the access token.
    """

    @current_app.route("/")
    @payment_required
    def index() -> Tuple[str, HTTPStatus]:
        return HTTPStatus.OK.phrase, HTTPStatus.OK

    user = make_user()

    client.login(user, admin=admin)

    assert client.get("/").status_code == status_code
