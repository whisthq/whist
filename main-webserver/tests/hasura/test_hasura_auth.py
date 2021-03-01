"""Tests for the /hasura/auth endpoint."""

import datetime

from datetime import datetime as dt
from http import HTTPStatus
from jose import jwt

from ..patches import function


def test_no_headers(client):
    response = client.get("/hasura/auth")
    assert response.status_code == HTTPStatus.OK
    assert response.json == {
        "X-Hasura-Role": "anonymous",
        "X-Hasura-User-Id": "",
        "X-Hasura-Login-Token": "",
    }


def test_login_header(client):
    response = client.get("/hasura/auth", headers={"X-Hasura-Login-Token": "login_token"})
    assert response.status_code == HTTPStatus.OK
    assert response.json == {
        "X-Hasura-Role": "anonymous",
        "X-Hasura-User-Id": "",
        "X-Hasura-Login-Token": "login_token",
    }


def test_auth_header(client, monkeypatch, make_authorized_user):
    response = client.get("/hasura/auth", headers={"Authorization": "Bearer auth_token"})
    assert response.status_code == HTTPStatus.OK
    assert response.json == {
        "X-Hasura-Role": "anonymous",
        "X-Hasura-User-Id": "",
        "X-Hasura-Login-Token": "",
    }

    authorized = make_authorized_user(
        created_timestamp=dt.now(datetime.timezone.utc).timestamp(),
    )
    monkeypatch.setattr(jwt, "decode", function(returns={"sub": authorized.user_id}))

    response = client.get("/hasura/auth", headers={"Authorization": "Bearer auth_token"})
    assert response.status_code == HTTPStatus.OK
    assert response.json == {
        "X-Hasura-Role": "user",
        "X-Hasura-User-Id": authorized.user_id,
        "X-Hasura-Login-Token": "",
    }
