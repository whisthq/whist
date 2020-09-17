"""Tests for the /container/create endpoint."""

from app.celery.aws_ecs_creation import create_new_container
from app.helpers.blueprint_helpers.aws.aws_container_post import (
    BadAppError,
    preprocess_task_info,
)

from ..patches import apply_async


def bad_app(*args, **kwargs):
    raise BadAppError


def test_bad_app(client, monkeypatch):
    # I couldn't figure out how to patch a function that was defined at the
    # top level of a module. I found this solution at:
    # https://stackoverflow.com/questions/28403380/py-tests-monkeypatch-setattr-not-working-in-some-cases
    monkeypatch.setattr(preprocess_task_info, "__code__", bad_app.__code__)
    client.login("new-email@fractalcomputers.com", "new-email-password")

    response = client.container_create("Bad App")

    assert response.status_code == 400


def test_no_username(client):
    client.login("new-email@fractalcomputers.com", "new-email-password")

    response = client.container_create(omit_username=True)

    assert response.status_code == 401


def test_no_app(client):
    client.login("new-email@fractalcomputers.com", "new-email-password")

    response = client.container_create(omit_app=True)

    assert response.status_code == 400


def test_successful(client, monkeypatch):
    monkeypatch.setattr(create_new_container, "apply_async", apply_async)
    client.login("new-email@fractalcomputers.com", "new-email-password")

    response = client.container_create("Blender")

    assert response.status_code == 202
