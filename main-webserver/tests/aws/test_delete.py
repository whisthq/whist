"""Tests for the /container/delete endpoint."""

import uuid

from http import HTTPStatus

import pytest

from app.celery.aws_ecs_deletion import delete_container
from app.helpers.utils.aws import aws_resource_locks
from app.helpers.utils.aws.base_ecs_client import ECSClient

from ..patches import function, Object


def test_no_container_id(client):
    response = client.post("/container/delete", json=dict(private_key="garbage!"))

    assert response.status_code == HTTPStatus.BAD_REQUEST


def test_no_key(client):
    response = client.post("/container/delete", json=dict(container_id="mycontainerid123"))

    assert response.status_code == HTTPStatus.BAD_REQUEST


def test_successful(client, authorized, monkeypatch):
    obj = Object()

    monkeypatch.setattr(delete_container, "apply_async", function(returns=obj))
    monkeypatch.setattr(obj, "id", str(uuid.uuid4()))

    response = client.post(
        "/container/delete", json=dict(container_id="mycontainerid123", private_key="garbage!")
    )

    assert response.status_code == HTTPStatus.ACCEPTED


def test_unauthorized(container, monkeypatch):
    c = container()

    with pytest.raises(Exception):
        delete_container(c.container_id, "garbage!")
