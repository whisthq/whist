"""A custom subclass of the Flask testing client."""

from flask.testing import FlaskClient
from flask_jwt_extended import create_access_token


class FractalClient(FlaskClient):
    """A custom test client class that makes it easy to authenticate requests."""

    def login(self, username):
        """Authenticate all future requests as the user with the specified username.

        Args:
            username: The user ID of the user as whom to authenticate all future requests.

        Returns:
            None
        """

        access_token = create_access_token(identity=username, expires_delta=False)
        self.environ_base["HTTP_AUTHORIZATION"] = f"Bearer {access_token}"

    def logout(self):
        """Ensure that all future requests are unauthenticated.

        Returns:
            None
        """

        del self.environ_base["HTTP_AUTHORIZATION"]
