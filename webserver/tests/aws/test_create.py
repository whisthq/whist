"""Tests for the /container/create endpoint."""

from app.celery.aws_ecs_creation import create_new_container
from app.helpers.blueprint_helpers.aws.aws_container_post import (
    BadAppError,
    preprocess_task_info,
)

from ..patches import apply_async


def bad_app(*args, **kwargs):
    raise BadAppError


def test_bad_app(client, authorized, monkeypatch):
    # I couldn't figure out how to patch a function that was defined at the
    # top level of a module. I found this solution at:
    # https://stackoverflow.com/questions/28403380/py-tests-monkeypatch-setattr-not-working-in-some-cases
    monkeypatch.setattr(preprocess_task_info, "__code__", bad_app.__code__)

    response = client.post(
        "/container/create", json=dict(username=authorized.user_id, app="Bad App")
    )

    assert response.status_code == 400


def test_no_username(client, authorized):
    response = client.post("/container/create", json=dict(app="VSCode"))

    assert response.status_code == 401


def test_no_app(client, authorized):
    response = client.post("/container/create", json=dict(username=authorized.user_id))

    assert response.status_code == 400


def test_no_region(client, authorized):
    response = client.post(
        "/container/create", json=dict(username=authorized.user_id, app="VSCode")
    )


def test_successful(client, authorized, monkeypatch):
    monkeypatch.setattr(create_new_container, "apply_async", apply_async)

    response = client.post(
        "/container/create",
        json=dict(username=authorized.user_id, app="Blender", region="us-east-1"),
    )

    assert response.status_code == 202
