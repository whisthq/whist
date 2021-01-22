"""Tests for the function that modifies the oauth.credentials table in the database."""

import functools

import pytest

from app.models import db
from app.blueprints.oauth import _provider_id_to_app_name, put_credential

from ..patches import function


def test_create_credential(make_token, provider, user):
    """Create a test user's first credential."""

    token = make_token()
    credential = put_credential(user.user_id, provider, token)

    assert len(user.credentials) == 1
    assert credential.user == user
    assert credential.access_token == token.access_token
    assert credential.expiry == token.expiry
    assert credential.provider_id == provider
    assert credential.refresh_token == token.refresh_token
    assert credential.token_type == token.token_type

    db.session.delete(credential)
    db.session.commit()


def test_overwrite_credential(make_credential, make_token, provider, user):
    """Replace an existing credential with a new one."""

    credential = make_credential(provider, cleanup=False)

    assert len(user.credentials) == 1
    assert credential.user == user

    db.session.expire(credential)

    credential = put_credential(user.user_id, provider, make_token())

    assert len(user.credentials) == 1
    assert credential.user == user

    db.session.delete(credential)
    db.session.commit()


def test_update_credential(make_credential, make_token, provider, user):
    """Update an existing credential with a new one."""

    token = make_token(refresh=False)
    credential = make_credential(provider)
    refresh_token = credential.refresh_token

    db.session.expire(credential)

    assert len(user.credentials) == 1

    credential = put_credential(user.user_id, provider, token)

    assert len(user.credentials) == 1
    assert credential.user == user
    assert credential.access_token == token.access_token
    assert credential.expiry == token.expiry
    assert credential.provider_id == provider
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
def test_list_connected_apps(client, make_credential, provider):
    """Return a list of connected applications."""

    make_credential(provider)

    response = client.get("/connected_apps")
    app_name = _provider_id_to_app_name(provider)

    assert response.json == {"app_names": [app_name]}


@pytest.mark.usefixtures("authorized")
def test_disconnect_app(client, make_credential, monkeypatch, provider):
    """Disconnect an external application from the test user's Fractal account."""

    credential = make_credential(provider)

    monkeypatch.setattr(credential, "revoke", function())

    app_name = _provider_id_to_app_name(provider)
    response = client.delete(f"/connected_apps/{app_name}")

    assert response.status_code == 200
    assert response.json == {}


@pytest.mark.usefixtures("authorized")
def test_disconnect_bad_app(client, provider):
    """Attempt to disconnect an external application that is already disconnected."""

    app_name = _provider_id_to_app_name(provider)
    response = client.delete(f"/connected_apps/{app_name}")

    assert response.status_code == 400
