"""Tests for the access control helpers check_payment() and @payment_required"""

from http import HTTPStatus

import pytest
import stripe

from flask import current_app, Flask
from flask_jwt_extended import JWTManager

from app.helpers.utils.general.limiter import limiter
from payments import check_payment, payment_required, PaymentRequired
from tests.client import FractalAPITestClient
from tests.patches import function


@pytest.fixture
def app():
    """Create a test Flask application.

    Returns:
        A Flask application instance.
    """

    _app = Flask(__name__)
    _app.secret_key = "secret"
    _app.test_client_class = FractalAPITestClient

    @_app.errorhandler(PaymentRequired)
    def _handle_payment_required(_error):
        return HTTPStatus.PAYMENT_REQUIRED.phrase, HTTPStatus.PAYMENT_REQUIRED

    JWTManager(_app)
    limiter.init_app(_app)

    return _app


@pytest.mark.parametrize("stripe_customer_id", (None, ""))
def test_check_payment_falsy(stripe_customer_id):
    """Ensure that check_payment() raises PaymentRequired when the customer ID is falsy."""

    with pytest.raises(PaymentRequired):
        check_payment(stripe_customer_id)


def test_check_payment_invalid(monkeypatch):
    """Ensure that check_payment() raises PaymentRequired when there is no valid subscription.

    Subscriptions are considered valid if and only if they are in the "active" or "trialing"
    states.
    """

    monkeypatch.setattr(
        stripe.Customer, "retrieve", function(returns={"subscriptions": [{"status": "garbage"}]})
    )

    with pytest.raises(PaymentRequired):
        check_payment("jeff")


@pytest.mark.parametrize("subscription_status", ("active", "trialing"))
def test_check_payment_valid(monkeypatch, subscription_status):
    """Ensure that check_payment() returns when there is a valid subscription."""

    monkeypatch.setattr(
        stripe.Customer,
        "retrieve",
        function(returns={"subscriptions": [{"status": subscription_status}]}),
    )

    check_payment("jeff")


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

    client.login(user, **{"https://api.fractal.co/stripe_customer_id": "cus_test"})
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

    client.login(user, subscribed=False, **login_kwargs)

    assert client.get("/").status_code == status_code
