"""Tests for the /container/ping endpoint."""

from app.helpers.blueprint_helpers.aws.aws_container_post import pingHelper


def status(code):
    return eval(f"""lambda *args, **kwargs: {{"status": {code}}}""")


def test_bad_request(client):
    client.login("new-email@fractalcomputers.com", "new-email-password")

    response = client.container_ping(omit_available=True)

    assert response.status_code == 400


def test_not_found(client, monkeypatch):
    code = 404

    monkeypatch.setattr(pingHelper, "__code__", status(code).__code__)
    client.login("new-email@fractalcomputers.com", "new-email-password")

    response = client.container_ping(available=True)

    assert response.status_code == code


def test_successful(client, monkeypatch):
    code = 200

    monkeypatch.setattr(pingHelper, "__code__", status(code).__code__)
    client.login("new-email@fractalcomputers.com", "new-email-password")

    response = client.container_ping(available=True)

    assert response.status_code == code
