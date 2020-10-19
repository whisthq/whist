"""Tests for the /container/protocol_info endpoint."""

from app.helpers.blueprint_helpers.aws.aws_container_post import protocol_info


def not_found(*args, **kwargs):
    return None, 404


def success(*args, **kwargs):
    return {}, 200


def test_not_found(client, monkeypatch):
    monkeypatch.setattr(protocol_info, "__code__", not_found.__code__)
    client.login("new-email@fractalcomputers.com", "new-email-password")

    response = client.container_info()

    assert response.status_code == 404


def test_successful(client, monkeypatch):
    monkeypatch.setattr(protocol_info, "__code__", success.__code__)
    client.login("new-email@fractalcomputers.com", "new-email-password")

    response = client.container_info()

    assert response.status_code == 200


def test_no_container():
    response, status = protocol_info("x.x.x.x", 0, 0)

    assert not response
    assert status == 404


def test_protocol_info(container):
    with container() as c:
        response, status = protocol_info(c.ip, c.port_32262, c.secret_key)

        assert status == 200
        assert response.pop("allow_autoupdate") == c.allow_autoupdate
        assert response.pop("branch") == c.branch
        assert response.pop("secret_key") == c.secret_key
        assert response.pop("using_stun") == c.using_stun
        assert not response  # The dictionary should be empty now.
