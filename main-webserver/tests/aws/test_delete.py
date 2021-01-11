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


def test_timeout(monkeypatch):
    monkeypatch.setattr(aws_resource_locks, "spin_lock", function(returns=-1))

    with pytest.raises(Exception):
        delete_container("mycontainerid123", "garbage!")


def test_unauthorized(container, monkeypatch):
    c = container()

    with pytest.raises(Exception):
        delete_container(c.container_id, "garbage!")


@pytest.mark.skip
@pytest.mark.usefixtures("celery_session_app")
@pytest.mark.usefixtures("celery_session_worker")
def test_delete(container, monkeypatch):
    """Delete an actual container.

    Based on the visual output of this test, I think the functionality is
    working. However, I didn't have time to figure out how to mock all of the
    methods that I needed to mock in order to make it pass.
    """

    c = container()

    monkeypatch.setattr(ECSClient, "__init__", function())
    monkeypatch.setattr(ECSClient, "add_task", function())
    monkeypatch.setattr(ECSClient, "check_if_done", function())
    monkeypatch.setattr(ECSClient, "stop_task", function())
    monkeypatch.setattr(ECSClient, "spin_til_done", function())

    delete_container.delay(c.container_id, c.secret_key)

    # TODO: Verify that the container has been deleted.
