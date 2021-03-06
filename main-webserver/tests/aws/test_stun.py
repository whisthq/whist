"""Tests for the /container/stun endpoint."""

import importlib
import uuid

from http import HTTPStatus

import app

from app.helpers.blueprint_helpers.aws import aws_container_post
from app.helpers.blueprint_helpers.aws.aws_container_post import set_stun

from ..patches import function


def test_no_container_id(client, authorized):
    response = client.post("/container/stun", json=dict(username=authorized.user_id, stun=True))

    assert response.status_code == HTTPStatus.BAD_REQUEST


def test_no_stun(client, authorized):
    response = client.post(
        "/container/stun", json=dict(username=authorized.user_id, container_id="mycontainerid123")
    )

    assert response.status_code == HTTPStatus.BAD_REQUEST


def test_no_username(client, authorized):
    response = client.post("/container/stun", json=dict(container_id="mycontainerid123", stun=True))

    assert response.status_code == HTTPStatus.UNAUTHORIZED


def test_not_found(client, authorized, monkeypatch):
    code = HTTPStatus.NOT_FOUND

    monkeypatch.setattr(aws_container_post, "set_stun", function(returns=code))

    response = client.post(
        "/container/stun",
        json=dict(username=authorized.user_id, container_id="mycontainerid123", stun=True),
    )

    assert response.status_code == code


def test_successful(client, authorized, monkeypatch):
    code = HTTPStatus.OK

    monkeypatch.setattr(aws_container_post, "set_stun", function(returns=code))
    importlib.reload(app.blueprints.aws.aws_container_blueprint)

    response = client.post(
        "/container/stun",
        json=dict(username=authorized.user_id, container_id="mycontainerid123", stun=True),
    )

    assert response.status_code == code


def test_no_container():
    result = set_stun(f"test-user-{uuid.uuid4()}", f"test-container-{uuid.uuid4()}", True)

    assert result == HTTPStatus.NOT_FOUND


def test_bad_user(container):
    with container() as c:
        result = set_stun(f"test-user-{uuid.uuid4()}", c.container_id, True)

    assert result == HTTPStatus.NOT_FOUND


def test_set_stun(container):
    with container() as c:
        result_0 = set_stun(c.user_id, c.container_id, True)

        assert result_0 == HTTPStatus.OK
        assert c.using_stun

        result_1 = set_stun(c.user_id, c.container_id, False)

        assert result_1 == HTTPStatus.OK
        assert not c.using_stun

        result_2 = set_stun(c.user_id, c.container_id, True)

        assert result_2 == HTTPStatus.OK
        assert c.using_stun
