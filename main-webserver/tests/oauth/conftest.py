"""Test fixtures useful for testing OAuth-related code."""

from datetime import datetime, timezone

import pytest

from app.models import Credential, db
from app.blueprints.oauth import Token


@pytest.fixture
def make_credential(make_token, user):
    """Expose a function that allows test code to create test credentials.

    Arguments:
        make_token: A function that generates a fake OAuth token for testing purposes.
        user: An instance of the User model.

    Returns:
        A function that adds test data to the configuration database.
    """

    credentials = []

    def _credential(cleanup=True):
        """Create a test row in the oauth.credentials table in the database.

        Keyword arguments:
            cleanup: Indicates whether or not to delete the credential at the end of this fixture's
                lifecycle.

        Returns:
            An instance of the Credential model.
        """

        token = make_token()
        credential = Credential(
            access_token=token.access_token,
            expiry=token.expiry,
            refresh_token=token.refresh_token,
            token_type=token.token_type,
            user_id=user.user_id,
        )

        db.session.add(credential)
        db.session.commit()

        if cleanup:
            credentials.append(credential)

        return credential

    yield _credential

    for credential in credentials:
        db.session.delete(credential)

    db.session.commit()


@pytest.fixture
def make_token():
    """Expose a function that generates a new fake OAuth token for testing purposes.

    Returns:
        A function that generates fake OAuth tokens.
    """

    def _token(refresh=True):
        """Generate a random OAuth token for testing purposes.

        Keyword arguments:
            refresh: A boolean indicating whether or not to include a refresh token.

        Returns:
            An instance of the Token namedtuple.
        """

        return Token(
            "access_token", datetime.now(timezone.utc), *(("refresh_token",) if refresh else ())
        )

    return _token
