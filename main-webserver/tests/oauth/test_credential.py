"""Tests for the Credential data model."""

from datetime import datetime, timezone

import pytest
import requests

from dropbox import Dropbox
from google_auth_oauthlib.flow import Flow
from oauthlib.oauth2 import InvalidGrantError

from app.models.oauth import _provider_id_to_app_name
from app.serializers.oauth import CredentialSchema

from ..patches import function, Object


@pytest.fixture
def dropbox_oauth_refresh(make_credential, monkeypatch, user):
    """Mock the parts of the dropbox API used by the Credential.refresh method.

    The goal is to construct a dummy instance of the Dropbox class.

    Returns:
        An instance of the Credential class.
    """

    credential = make_credential(user.user_id, "dropbox")
    dropbox = Object()

    monkeypatch.setattr(Dropbox, "__new__", function(returns=dropbox))
    monkeypatch.setattr(dropbox, "refresh_access_token", function())
    monkeypatch.setattr(dropbox, "_oauth2_access_token", "new_access_token")
    monkeypatch.setattr(dropbox, "_oauth2_access_token_expiration", datetime.now(timezone.utc))
    monkeypatch.setattr(dropbox, "_oauth2_refresh_token", credential.refresh_token)

    return credential


@pytest.fixture
def dropbox_oauth_revoke(monkeypatch):
    """Mock the parts of the dropbox API used by the Credential.revoke method.

    Returns:
        None
    """

    dropbox = Object()

    monkeypatch.setattr(Dropbox, "__new__", function(returns=dropbox))
    monkeypatch.setattr(dropbox, "auth_token_revoke", function())


@pytest.fixture
def google_oauth_refresh(make_credential, monkeypatch, user):
    """Mock the parts of the google-auth-oauthlib API used by the Credential.refresh method.

    The goal is to construct a dummy instance of the google-auth-oauthlib Flow class. This is a
    complicated test fixture, but it is easier to understand if you read it and the implementation
    of Credential.refresh simultaneously.

    Returns:
        An instance of the Credential class.
    """

    # Set only the keys we're going to need to access when we call the refresh_token method.
    client_config = {
        "token_uri": "",
        "client_id": "",
        "client_secret": "",
    }
    credentials = Object()  # Mocks the credentials attribute of the Flow instance.
    flow = Object()  # Mocks the flow instance itself.
    session = Object()  # Mocks the underlying OAuth2Session instance.

    credential = make_credential(user.user_id, "google")

    monkeypatch.setattr(Flow, "from_client_config", function(returns=flow))

    # The google-auth-oauthlib isn't super feature-complete, so we have to access the refresh_token
    # method, which returns None, on the underlying OAut2Session instance.
    monkeypatch.setattr(flow, "oauth2session", session)
    monkeypatch.setattr(session, "refresh_token", function())

    # We have to read some values from the client_config dictionary, which is an attribute of the
    # Flow instance, when constructing our call to the refresh_token method
    monkeypatch.setattr(flow, "client_config", client_config)

    # Construct the mock credentials object from which we read the refreshed token values. The
    # value of the token attribute on the credentials object has to be
    monkeypatch.setattr(flow, "credentials", credentials)
    monkeypatch.setattr(credentials, "expiry", datetime.now(timezone.utc))
    monkeypatch.setattr(credentials, "refresh_token", credential.refresh_token)
    monkeypatch.setattr(credentials, "token", "new_access_token")

    return credential


@pytest.fixture
def google_oauth_revoke(monkeypatch):
    """Mock the parts of the google-auth-oauthlib library used by the Credential.revoke method.

    Returns:
        None
    """

    response = Object()

    monkeypatch.setattr(requests, "post", function(returns=response))
    monkeypatch.setattr(response, "ok", True)


def test_refresh(provider, request):
    """Verify that the refresh method refreshes an access token.

    Arguments:
        make_credential: A function that adds test rows to the credentials table of the database.
        monkeypatch: The built-in monkeypatch test fixture.
    """

    credential = request.getfixturevalue(f"{provider}_oauth_refresh")
    expiry = credential.expiry

    credential.refresh(force=True)

    assert credential.access_token == "new_access_token"
    assert credential.expiry > expiry


def test_refresh_expired(make_credential, monkeypatch, user):
    """Verify that the refresh method raises InvalidGrantError for expired Google OAuth tokens.

    Currently, there is only a Google version of this test.
    """

    client_config = dict(token_uri="", client_id="", client_secret="")
    flow = Object()
    oauth2session = Object()

    credential = make_credential(user.user_id, "google")

    monkeypatch.setattr(flow, "client_config", client_config)
    monkeypatch.setattr(flow, "oauth2session", oauth2session)
    monkeypatch.setattr(Flow, "from_client_config", function(returns=flow))
    monkeypatch.setattr(oauth2session, "refresh_token", function(raises=InvalidGrantError))

    with pytest.raises(InvalidGrantError):
        credential.refresh(cleanup=False, force=True)


def test_revoke(make_credential, provider, request, user):
    """Simulate token revocation."""

    credential = make_credential(user.user_id, provider)

    request.getfixturevalue(f"{provider}_oauth_revoke")
    credential.revoke(cleanup=False)


def test_revoke_error(make_credential, user):
    """Handle an error response from the Google OAuth token revocation endpoint.

    Currently, there is only a Google version of this test.
    """

    credential = make_credential(user.user_id, "google")

    credential.revoke(cleanup=False)


def test_serialize(make_credential, provider, user):
    """Make sure the CredentialSchema serializer serializes provider_id correctly."""

    schema = CredentialSchema()
    credential = make_credential(user.user_id, provider)
    obj = schema.dump(credential)

    assert obj == {
        "access_token": credential.access_token,
        "provider": _provider_id_to_app_name(credential.provider_id),
        "expiry": credential.expiry.isoformat(),
        "refresh_token": credential.refresh_token,
        "token_type": credential.token_type,
    }
