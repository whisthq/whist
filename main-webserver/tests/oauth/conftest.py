"""Test fixtures useful for testing OAuth-related code."""

from datetime import datetime, timezone

import pytest

from app.models import Credential, db
from app.blueprints.oauth import Token


@pytest.fixture
def make_credential(make_token):
    """Create a test row in the oauth.credentials table in the database.

    Args:
        username: The user ID of the Fractal user with whose account to associate the credential.
        provider: The provider ID of the OAuth provider for which to generate the fake credential.
        cleanup: Optional. A boolean indicating whether or not to delete the fake credential at the
            end of this test fixture's lifecycle.

    Returns:
        A function that adds test data to the configuration database.
    """

    credentials = []

    def _credential(username, provider, cleanup=True):
        token = make_token()
        credential = Credential(
            access_token=token.access_token,
            expiry=token.expiry,
            provider_id=provider,
            refresh_token=token.refresh_token,
            token_type=token.token_type,
            user_id=username,
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


@pytest.fixture
def oauth_client_disabled(app, monkeypatch):
    """Delete the test application's dummy OAuth client credentials.

    Returns:
        None
    """

    monkeypatch.setitem(app.config, "DROPBOX_APP_KEY", None)
    monkeypatch.setitem(app.config, "DROPBOX_APP_SECRET", None)
    monkeypatch.setitem(app.config, "GOOGLE_CLIENT_SECRET_OBJECT", {})


@pytest.fixture(params=("dropbox", "google"))
def provider(request):
    return request.param
