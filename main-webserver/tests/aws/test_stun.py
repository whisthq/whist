"""Tests for the /container/stun endpoint."""

from app.helpers.blueprint_helpers.aws.aws_container_post import set_stun


def status(code):
    return eval(f"""lambda *args, **kwargs: {code}""")


def test_no_container_id(client):
    client.login("new-email@fractalcomputers.com", "new-email-password")

    response = client.container_stun(stun=True, omit_container_id=True)

    assert response.status_code == 400


def test_no_stun(client):
    client.login("new-email@fractalcomputers.com", "new-email-password")

    response = client.container_stun(container_id="mycontainerid123", omit_stun=True)

    assert response.status_code == 400


def test_no_username(client):
    client.login("new-email@fractalcomputers.com", "new-email-password")

    response = client.container_stun(
        container_id="mycontainerid123", stun=True, omit_username=True
    )

    assert response.status_code == 401


def test_not_found(client, monkeypatch):
    code = 404

    monkeypatch.setattr(set_stun, "__code__", status(code).__code__)
    client.login("new-email@fractalcomputers.com", "new-email-password")

    response = client.container_stun(container_id="mycontainerid123", stun=True)

    assert response.status_code == code


def test_successful(client, monkeypatch):
    code = 200

    monkeypatch.setattr(set_stun, "__code__", status(code).__code__)
    client.login("new-email@fractalcomputers.com", "new-email-password")

    response = client.container_stun(container_id="mycontainerid123", stun=True)

    assert response.status_code == code
