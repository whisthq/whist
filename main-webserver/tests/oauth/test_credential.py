"""Tests for the Credential data model."""

from datetime import datetime, timezone

import pytest
import requests

from google_auth_oauthlib.flow import Flow
from oauthlib.oauth2 import InvalidGrantError

from ..patches import function, Object


def test_refresh(make_credential, monkeypatch):
    """Verify that the refresh method refreshes an access token.

    Arguments:
        make_credential: A function that adds test rows to the credentials table of the database.
        monkeypatch: The built-in monkeypatch test fixture.
    """

    client_config = dict(token_uri="", client_id="", client_secret="")
    credentials = Object()
    flow = Object()
    oauth2session = Object()

    credential = make_credential()
    expiry = credential.expiry

    monkeypatch.setattr(credentials, "expiry", datetime.now(timezone.utc), raising=False)
    monkeypatch.setattr(credentials, "refresh_token", credential.refresh_token, raising=False)
    monkeypatch.setattr(credentials, "token", "new_access_token", raising=False)
    monkeypatch.setattr(flow, "client_config", client_config, raising=False)
    monkeypatch.setattr(flow, "credentials", credentials, raising=False)
    monkeypatch.setattr(flow, "oauth2session", oauth2session, raising=False)
    monkeypatch.setattr(Flow, "from_client_config", function(returns=flow))
    monkeypatch.setattr(oauth2session, "refresh_token", function(), raising=False)

    credential.refresh(force=True)

    assert credential.access_token == "new_access_token"
    assert credential.expiry > expiry


def test_refresh_expired(make_credential, monkeypatch):
    """Verify that the refresh method raises InvalidGrantError when it encounters an expired token.

    Arguments:
        make_credential: A function that adds test rows to the credentials table of the database.
        monkeypatch: The built-in monkeypatch test fixture.
    """

    client_config = dict(token_uri="", client_id="", client_secret="")
    flow = Object()
    oauth2session = Object()

    credential = make_credential()

    monkeypatch.setattr(flow, "client_config", client_config, raising=False)
    monkeypatch.setattr(flow, "oauth2session", oauth2session, raising=False)
    monkeypatch.setattr(Flow, "from_client_config", function(returns=flow), raising=False)
    monkeypatch.setattr(
        oauth2session, "refresh_token", function(raises=InvalidGrantError), raising=False
    )

    with pytest.raises(InvalidGrantError):
        credential.refresh(cleanup=False, force=True)


def test_revoke(make_credential, monkeypatch):
    """Simulate token revocation.

    Arguments:
        make_credential: A function that adds test rows to the credentials table of the database.
        monkeypatch: The build-in monkeypatch test fixture.
    """

    response = Object()

    credential = make_credential()

    monkeypatch.setattr(response, "ok", True, raising=False)
    monkeypatch.setattr(requests, "post", function(returns=response))

    credential.revoke(cleanup=False)


def test_revoke_error(make_credential):
    """Handle an error response from the token revocation endpoint.

    Arguments:
        make_credential: A function that adds test rows to the credentials table of the database.
        monkeypatch: The build-in monkeypatch test fixture.
    """

    credential = make_credential()

    credential.revoke(cleanup=False)
