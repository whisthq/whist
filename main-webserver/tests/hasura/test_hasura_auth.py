"""Tests for the /hasura/auth endpoint."""

from http import HTTPStatus
from flask_jwt_extended import create_access_token


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


def test_auth_header(client, make_authorized_user):
    authorized = make_authorized_user()
    access_token = create_access_token(authorized)

    response = client.get(
        "/hasura/auth", headers={"Authorization": "Bearer {}".format(access_token)}
    )
    assert response.status_code == HTTPStatus.OK
    assert response.json == {
        "X-Hasura-Role": "user",
        "X-Hasura-User-Id": authorized,
        "X-Hasura-Login-Token": "",
    }
