"""Tests for the function that modifies the oauth.credentials table in the database."""

import functools

from datetime import datetime, timezone

import pytest

from app.models import Credential, db
from app.blueprints.oauth import Token
from app.blueprints.oauth import put_credential as _put_credential


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


@pytest.fixture
def put_credential(user):
    """Automatically populate the user_id argument to the put_credential function.

    Arguments:
        user: An instance of the User model.

    Returns:
        The partial application of the put_credential function to its first argument, the user_id
        of the user who owns the credential.
    """

    return functools.partial(_put_credential, user.user_id)


def test_create_credential(make_token, put_credential, user):
    """Create a test user's first credential.

    Arguments:
        make_token: A function that generates a fake OAuth token for testing purposes.
        put_credential: A wrapper around the real put_credential function that does not require a
            user_id as an argument.
        user: An instance of the User model.
    """

    token = make_token()
    credential = put_credential(token)

    assert len(user.credentials) == 1
    assert credential.user == user
    assert credential.access_token == token.access_token
    assert credential.expiry == token.expiry
    assert credential.refresh_token == token.refresh_token
    assert credential.token_type == token.token_type

    db.session.delete(credential)
    db.session.commit()


def test_overwrite_credential(make_credential, make_token, put_credential, user):
    """Replace an existing credential with a new one.

    Arguments:
        make_credential: A function that adds test rows to the oauth.credentials table.
        make_token: A function that generates fake OAuth token for testing purposes.
        put_credential: A wrapper around the real put_credential function that does not require a
            user_id as an argument.
        user: an instance of the User model.
    """

    credential = make_credential(cleanup=False)

    assert len(user.credentials) == 1
    assert credential.user == user

    db.session.expire(credential)

    credential = put_credential(make_token())

    assert len(user.credentials) == 1
    assert credential.user == user

    db.session.delete(credential)
    db.session.commit()


def test_update_credential(make_credential, make_token, put_credential, user):
    """Update an existing credential with a new one.

    Arguments:
        make_credential: A function that adds test rows to the oauth.credentials table.
        make_token: A function that generates fake OAuth token for testing purposes.
        put_credential: A wrapper around the real put_credential function that does not require a
            user_id as an argument.
        user: an instance of the User model.
    """

    token = make_token(refresh=False)
    credential = make_credential()
    refresh_token = credential.refresh_token

    db.session.expire(credential)

    assert len(user.credentials) == 1

    credential = put_credential(token)

    assert len(user.credentials) == 1
    assert credential.user == user
    assert credential.access_token == token.access_token
    assert credential.expiry == token.expiry
    assert credential.refresh_token == refresh_token
    assert credential.token_type == token.token_type


@pytest.mark.usefixtures("authorized")
def test_list_no_connected_apps(client):
    """Return an empty list of connected external applications.

    Arguments:
        client: An instance of the Flask test client.
    """

    response = client.get("/connected_apps")

    assert response.json == {"app_names": []}


@pytest.mark.usefixtures("authorized")
def test_list_connected_apps(client, make_credential):
    """Return a list of connected applications.

    Arguments:
        client: An instance of the Flask test client.
        make_credential: A function that adds test rows to the oauth.credentials table.
    """

    make_credential()

    response = client.get("/connected_apps")

    assert response.json == {"app_names": ["google_drive"]}


@pytest.mark.usefixtures("authorized")
def test_disconnect_app(client, make_credential):
    """Disconnect an external application from the test user's Fractal account.

    Arguments:
        client: An instance of the Flask test client.
        make_credential: A function that adds test rows to the oauth.credentials table.
    """

    # TODO: Patch over the request to revoke the test user's fake access token so it doesn't actually
    # get sent.

    make_credential(cleanup=False)

    first_response = client.delete("/connected_apps/google_drive")

    assert first_response.status_code == 200

    second_response = client.get("/connected_apps")

    assert second_response.json == {"app_names": []}


@pytest.mark.usefixtures("authorized")
def test_disconnect_bad_app(client):
    """Attempt to disconnect an external application that is already disconnected.

    Arguments:
        client: An instance of the Flask test client.
    """

    response = client.delete("/connected_apps/google_drive")

    assert response.status_code == 400
