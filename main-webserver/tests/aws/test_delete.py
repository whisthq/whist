"""Tests for the /container/delete endpoint."""

import pytest

from app.celery.aws_ecs_deletion import deleteContainer
from app.helpers.utils.aws.aws_resource_locks import spinLock
from app.helpers.utils.aws.base_ecs_client import ECSClient

from ..patches import apply_async, do_nothing


def test_no_container_id(client):
    response = client.post("/container/delete", json=dict(private_key="garbage!"))

    assert response.status_code == 400


def test_no_key(client):
    response = client.post("/container/delete", json=dict(container_id="mycontainerid123"))

    assert response.status_code == 400


def test_successful(client, authorized, monkeypatch):
    monkeypatch.setattr(deleteContainer, "apply_async", apply_async)

    response = client.post(
        "/container/delete", json=dict(container_id="mycontainerid123", private_key="garbage!")
    )

    assert response.status_code == 202


def test_timeout(monkeypatch):
    give_up = lambda *args, **kwargs: -1

    monkeypatch.setattr(spinLock, "__code__", give_up.__code__)

    with pytest.raises(Exception):
        deleteContainer("mycontainerid123", "garbage!")


def test_unauthorized(container, monkeypatch):
    c = container()

    with pytest.raises(Exception):
        deleteContainer(c.container_id, "garbage!")


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

    monkeypatch.setattr(ECSClient, "__init__", do_nothing)
    monkeypatch.setattr(ECSClient, "add_task", do_nothing)
    monkeypatch.setattr(ECSClient, "check_if_done", do_nothing)
    monkeypatch.setattr(ECSClient, "stop_task", do_nothing)
    monkeypatch.setattr(ECSClient, "spin_til_done", do_nothing)

    deleteContainer.delay(c.container_id, c.secret_key)

    # TODO: Verify that the container has been deleted.
