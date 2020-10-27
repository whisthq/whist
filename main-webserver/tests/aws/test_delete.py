"""Tests for the /container/delete endpoint."""

from app.celery.aws_ecs_deletion import deleteContainer

from ..patches import apply_async


def test_no_container_id(client, authorized):
    response = client.post("/container/delete", json=dict(username=authorized.user_id))

    assert response.status_code == 400


def test_no_username(client, authorized):
    response = client.post("/container/delete", json=dict(container_id="mycontainerid123"))

    assert response.status_code == 401


def test_successful(client, authorized, monkeypatch):
    monkeypatch.setattr(deleteContainer, "apply_async", apply_async)

    response = client.post(
        "/container/delete", json=dict(username=authorized.user_id, container_id="mycontainerid123")
    )

    assert response.status_code == 202
