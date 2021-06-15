"""A custom subclass of the Flask testing client."""

import os

from flask.testing import FlaskClient
from flask_jwt_extended import create_access_token

from payments import PaymentRequired


class FractalAPITestClient(FlaskClient):
    """A custom test client class that makes it easy to authenticate requests.

    Args:
        *args: Positional arguments that are forwarded directly to the superclass's constructor.
        request: The pytest FixtureRequest object representing the current requesting test context.
        **kwargs: Keyword arguments that are forwarded directly to the superclass's constructor.
    """

    def __init__(self, *args, request, **kwargs):
        super().__init__(*args, **kwargs)

        self._request = request

    def login(self, username, *, admin=False, subscribed=True, **additional_claims):
        """Authenticate all future requests as the user with the specified username.

        Args:
            username: The user ID of the user as whom to authenticate all future requests.
            admin: Optional. A boolean indicating whether or not the authenticated user should have
                have access to administrative endpoints.
            subscribed: Optional. A boolean indicating whether or not the authenticated user should
                be treated as if they have a valid Fractal subscription.

        Returns:
            None
        """

        if admin:
            additional_claims["scope"] = "admin"

        if subscribed:
            stripe_customer_id = f"cus_test_{os.urandom(8).hex()}"
            monkeypatch = self._request.getfixturevalue("monkeypatch")
            additional_claims["https://api.fractal.co/stripe_customer_id"] = stripe_customer_id

            def check_payment(customer_id):
                if customer_id != stripe_customer_id:
                    raise PaymentRequired

            monkeypatch.setattr("payments.check_payment", check_payment)

        access_token = create_access_token(
            identity=username, additional_claims=additional_claims, expires_delta=False
        )

        self.environ_base["HTTP_AUTHORIZATION"] = f"Bearer {access_token}"

    def logout(self):
        """Ensure that all future requests are unauthenticated.

        Returns:
            None
        """

        del self.environ_base["HTTP_AUTHORIZATION"]
