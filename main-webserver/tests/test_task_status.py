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


@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
def test_nonexistent(client):
    """Make sure that task status queries for garbage task IDs receive Bad Request responses."""

    response = client.get("/status/garbage")

    assert response.status_code == HTTPStatus.BAD_REQUEST


def test_backend(celery_app):
    """Ensure that the patched FractalRedisBackend handles garbage task IDs properly.

    We want the patched backend to report that unregistered tasks are in a special null state, not
    the PENDING state.
    """

    assert celery_app.AsyncResult("hello").state is None
