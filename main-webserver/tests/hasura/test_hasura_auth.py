"""Tests for the /hasura/auth endpoint."""

import datetime

from datetime import datetime as dt
from http import HTTPStatus

import pytest

from flask_jwt_extended import create_access_token


@pytest.mark.skip
def test_no_headers(client):
    response = client.get("/hasura/auth")
    assert response.status_code == HTTPStatus.OK
    assert response.json == {
        "X-Hasura-Role": "anonymous",
        "X-Hasura-User-Id": "",
        "X-Hasura-Login-Token": "",
    }


@pytest.mark.skip
def test_login_header(client):
    response = client.get("/hasura/auth", headers={"X-Hasura-Login-Token": "login_token"})
    assert response.status_code == HTTPStatus.OK
    assert response.json == {
        "X-Hasura-Role": "anonymous",
        "X-Hasura-User-Id": "",
        "X-Hasura-Login-Token": "login_token",
    }


def test_auth_header(client, make_authorized_user):
    authorized = make_authorized_user(
        created_timestamp=dt.now(datetime.timezone.utc).timestamp(),
    )
    access_token = create_access_token(authorized.user_id)

    response = client.get(
        "/hasura/auth", headers={"Authorization": "Bearer {}".format(access_token)}
    )
    assert response.status_code == HTTPStatus.OK
    assert response.json == {
        "X-Hasura-Role": "user",
        "X-Hasura-User-Id": authorized.user_id,
        "X-Hasura-Login-Token": "",
    }
