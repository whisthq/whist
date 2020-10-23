"""Tests for the /container/stun endpoint."""

import uuid

from app.constants.http_codes import NOT_FOUND, SUCCESS
from app.helpers.blueprint_helpers.aws.aws_container_post import set_stun


def status(code):
    return eval(f"""lambda *args, **kwargs: {code}""")


def test_no_container_id(client, authorized):
    response = client.post("/container/stun", json=dict(username=authorized.user_id, stun=True))

    assert response.status_code == 400


def test_no_stun(client, authorized):
    response = client.post(
        "/container/stun", json=dict(username=authorized.user_id, container_id="mycontainerid123")
    )

    assert response.status_code == 400


def test_no_username(client, authorized):
    response = client.post("/container/stun", json=dict(container_id="mycontainerid123", stun=True))

    assert response.status_code == 401


def test_not_found(client, authorized, monkeypatch):
    code = 404

    monkeypatch.setattr(set_stun, "__code__", status(code).__code__)

    response = client.post(
        "/container/stun",
        json=dict(username=authorized.user_id, container_id="mycontainerid123", stun=True),
    )

    assert response.status_code == code


def test_successful(client, authorized, monkeypatch):
    code = 200

    monkeypatch.setattr(set_stun, "__code__", status(code).__code__)

    response = client.post(
        "/container/stun",
        json=dict(username=authorized.user_id, container_id="mycontainerid123", stun=True),
    )

    assert response.status_code == code


def test_no_container():
    result = set_stun(f"test-user-{uuid.uuid4()}", f"test-container-{uuid.uuid4()}", True)

    assert result == NOT_FOUND


def test_bad_user(container):
    with container() as c:
        result = set_stun(f"test-user-{uuid.uuid4()}", c.container_id, True)

        assert result == NOT_FOUND


def test_set_stun(container):
    with container() as c:
        result_0 = set_stun(c.user_id, c.container_id, True)

        assert result_0 == SUCCESS
        assert c.using_stun

        result_1 = set_stun(c.user_id, c.container_id, False)

        assert result_1 == SUCCESS
        assert not c.using_stun

        result_2 = set_stun(c.user_id, c.container_id, True)

        assert result_2 == SUCCESS
        assert c.using_stun
