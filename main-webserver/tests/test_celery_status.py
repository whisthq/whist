"""Tests for the /celery/<task-id> endpoint."""

import pytest

from celery import shared_task


@shared_task
def task():
    """Return immediately.

    Used for testing purposes.

    Returns:
        None
    """


@pytest.mark.parametrize("task_id", ("undefined", "garbage", "abcdefghijklmnopqrstuvwxyz012345"))
def test_invalid_task_id(client, task_id):
    """Ensure that the /status/<task-id> endpoint rejects task IDs that are not UUIDs."""

    response = client.get(f"/status/{task_id}")

    assert response.status_code == 400


@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
def test_valid_task_id(client):
    """Ensure that the /status/<task-id> endpoint accepts task IDs that are UUIDs."""

    result = task.delay()

    result.get()

    response = client.get(f"/status/{result}")

    assert response.status_code == 200
