"""Tests for the /container/ping endpoint."""

import os
import pytest

from app.constants.http_codes import SUCCESS
from app.helpers.blueprint_helpers.aws.aws_container_post import ping_helper
from app.models import db

from ..patches import Patch


def status_code(*args, **kwargs):
    from tests.patches import Patch

    patch = Patch()

    return None, patch.status_code


def test_no_availability(client):
    response = client.post("/container/ping", json=dict(identifier=0, private_key="aes_secret_key"))

    assert response.status_code == 400


def test_no_port(client):
    response = client.post(
        "/container/ping", json=dict(available=True, private_key="aes_secret_key")
    )

    assert response.status_code == 400


def test_no_key(client):
    response = client.post("container/ping", json=dict(available=True, identifier=0))

    assert response.status_code == 400


def test_not_found(client, monkeypatch):
    code = 404

    monkeypatch.setattr(Patch, "status_code", code, raising=False)
    monkeypatch.setattr(ping_helper, "__code__", status_code.__code__)

    response = client.post(
        "/container/ping", json=dict(available=True, identifier=0, private_key="aes_secret_key")
    )

    assert response.status_code == code


def test_successful(client, monkeypatch):
    code = 200

    monkeypatch.setattr(Patch, "status_code", code, raising=False)
    monkeypatch.setattr(ping_helper, "__code__", status_code.__code__)

    response = client.post(
        "/container/ping", json=dict(available=True, identifier=0, private_key="aes_secret_key")
    )

    assert response.status_code == code


def test_no_container():
    data, status = ping_helper(True, "x.x.x.x", 0, "garbage!")

    assert status == 404


def test_wrong_key(container):
    with container("RUNNING_AVAILABLE") as c:
        data, status = ping_helper(True, c.ip, c.port_32262, "garbage!")

    assert status == 404


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
        assert status == SUCCESS

        # I don't totally understand why this line is necessary, but it seems
        # to be. -O
        db.session.add(c)

        assert c.state == final_state

        history = c.user.history

        if initial_state == "RUNNING_AVAILABLE" and final_state == "RUNNING_UNAVAILABLE":
            # Make sure the login was recorded in the database.
            login = history.first()

            assert login
            assert login.action == "logon"
        elif initial_state == "RUNNING_UNAVAILABLE" and final_state == "RUNNING_AVAILABLE":
            # Make sure the logout was recorded in the database.
            logout = history.first()

            assert logout
            assert logout.action == "logoff"
        else:
            # Make sure that neither any login nor logout events have been
            # recorded.
            assert not db.session.query(history.exists()).scalar()
