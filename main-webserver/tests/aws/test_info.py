"""Tests for the /container/protocol_info endpoint."""

from app.helpers.blueprint_helpers.aws.aws_container_get import protocol_info


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
