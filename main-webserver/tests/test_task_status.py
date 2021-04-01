"""Tests that verify that the /status/<task-id> endpoint returns meaningful responses."""

from http import HTTPStatus

import pytest

from celery import shared_task


@shared_task
def task():
    """A trivial task used for testing purposes.

    Returns:
        None
    """

    pass


@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
def test_success(client):
    """Ensure that the task status endpoint returns OK when a task executes successfully."""

    result = task.delay()

    # Make sure the task has completely finished before we query the endpoint.
    assert result.get() is None

    response = client.get(f"/status/{result}")

    assert response.status_code == HTTPStatus.OK
    assert response.json == {"state": "SUCCESS", "output": None}


def test_backend(celery_app):
    """Ensure that the patched FractalRedisBackend handles garbage task IDs properly.

    We want the patched backend to report that unregistered tasks are in a special null state, not
    the PENDING state.
    """

    assert celery_app.AsyncResult("hello").state is None
