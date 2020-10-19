"""Tests for the /container/delete endpoint."""

from app.celery.aws_ecs_deletion import deleteContainer

from ..patches import apply_async


def test_no_container_id(client):
    client.login("new-email@tryfractal.com", "new-email-password")

    response = client.container_delete(omit_container_id=True)

    assert response.status_code == 400


def test_no_username(client):
    client.login("new-email@tryfractal.com", "new-email-password")

    response = client.container_delete(omit_username=True)

    assert response.status_code == 401


def test_successful(client, monkeypatch):
    monkeypatch.setattr(deleteContainer, "apply_async", apply_async)
    client.login("new-email@tryfractal.com", "new-email-password")

    response = client.container_delete(container_id="mycontainerid123")

    assert response.status_code == 202
