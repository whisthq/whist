"""Tests for the /container/ping endpoint."""

import importlib

from http import HTTPStatus

import pytest

import app

from app.helpers.blueprint_helpers.aws import aws_container_post
from app.helpers.blueprint_helpers.aws.aws_container_post import ping_helper
from app.models import db

from ..patches import function


def test_no_availability(client):
    response = client.post("/container/ping", json=dict(identifier=0, private_key="aes_secret_key"))

    assert response.status_code == HTTPStatus.BAD_REQUEST


def test_no_port(client):
    response = client.post(
        "/container/ping", json=dict(available=True, private_key="aes_secret_key")
    )

    assert response.status_code == HTTPStatus.BAD_REQUEST


def test_no_key(client):
    response = client.post("container/ping", json=dict(available=True, identifier=0))

    assert response.status_code == HTTPStatus.BAD_REQUEST


def test_not_found(client, monkeypatch):
    code = HTTPStatus.NOT_FOUND

    monkeypatch.setattr(aws_container_post, "ping_helper", function(returns=(None, code)))

    response = client.post(
        "/container/ping", json=dict(available=True, identifier=0, private_key="aes_secret_key")
    )

    assert response.status_code == code


def test_successful(client, monkeypatch):
    code = HTTPStatus.OK

    monkeypatch.setattr(aws_container_post, "ping_helper", function(returns=({}, code)))
    importlib.reload(app.blueprints.aws.aws_container_blueprint)

    response = client.post(
        "/container/ping", json=dict(available=True, identifier=0, private_key="aes_secret_key")
    )

    assert response.status_code == code


def test_no_container():
    data, status = ping_helper(True, "x.x.x.x", 0, "garbage!")

    assert status == HTTPStatus.NOT_FOUND


def test_wrong_key(container):
    with container("RUNNING_AVAILABLE") as c:
        data, status = ping_helper(True, c.ip, c.port_32262, "garbage!")

    assert status == HTTPStatus.NOT_FOUND


@pytest.mark.parametrize(
    "available, initial_state, final_state",
    (
        (True, "CREATING", "RUNNING_AVAILABLE"),
        (True, "RUNNING_AVAILABLE", "RUNNING_AVAILABLE"),
        (True, "RUNNING_UNAVAILABLE", "RUNNING_AVAILABLE"),
        (True, "STOPPING", "STOPPING"),
        (False, "CREATING", "RUNNING_UNAVAILABLE"),
        (False, "RUNNING_AVAILABLE", "RUNNING_UNAVAILABLE"),
        (False, "RUNNING_UNAVAILABLE", "RUNNING_UNAVAILABLE"),
        (False, "STOPPING", "STOPPING"),
    ),
)
def test_ping_helper(available, container, final_state, initial_state):
    with container(initial_state) as c:
        data, status = ping_helper(available, c.ip, c.port_32262, c.secret_key)

        assert data.pop("status", None) == "OK"
        assert not data
        assert status == HTTPStatus.OK

        # I don't totally understand why this line is necessary, but it seems
        # to be. -O
        db.session.add(c)

        assert c.state == final_state
