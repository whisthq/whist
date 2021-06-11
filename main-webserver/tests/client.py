"""A custom subclass of the Flask testing client."""

from flask.testing import FlaskClient
from flask_jwt_extended import create_access_token


class FractalAPITestClient(FlaskClient):
    """A custom test client class that makes it easy to authenticate requests."""

    def login(self, username, *, admin=False, **additional_claims):
        """Authenticate all future requests as the user with the specified username.

        Args:
            username: The user ID of the user as whom to authenticate all future requests.
            admin: Optional. A boolean indicating whether or not the authenticated user should have
                have access to administrative endpoints.

        Returns:
            None
        """

        if admin:
            additional_claims["scope"] = "admin"

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
