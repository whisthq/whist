"""Tests for code required by the /admin/logs endpoint."""

import uuid

from http import HTTPStatus

import boto3
import pytest

from app.celery.aws_s3_modification import (
    BadSenderError,
    ContainerNotFoundError,
    upload_logs_to_s3,
)

from ..patches import function, Object


def test_bad_sender():
    with pytest.raises(BadSenderError):
        upload_logs_to_s3("USPS", "x.x.x.x", "", "Log message.")


def test_no_container():
    with pytest.raises(ContainerNotFoundError):
        upload_logs_to_s3("client", "x.x.x.x", "", "Log message.")


def test_unauthorized(container):
    with container() as c:
        with pytest.raises(ContainerNotFoundError):
            upload_logs_to_s3(
                "client",
                c.container_id,
                "garbage!",
                "Log message.",
            )


def test_s3_failure(container, monkeypatch):
    obj = Object()

    monkeypatch.setattr(boto3, "resource", function(returns=obj))
    monkeypatch.setattr(obj, "put", function(raises=Exception))

    with container() as c:
        with pytest.raises(Exception):
            upload_logs_to_s3(
                "client",
                c.container_id,
                c.secret_key,
                "Log message.",
            )


@pytest.mark.parametrize("sender", ("client", "server"))
def test_s3_success(container, sender, monkeypatch):
    obj = Object()
    resource = Object()
    is_client = sender == "client"
    connection_id = str(uuid.uuid4())

    monkeypatch.setattr(boto3, "resource", function(returns=resource))
    monkeypatch.setattr(obj, "put", function())
    monkeypatch.setattr(resource, "Object", function(returns=obj))

    with container() as c:
        response = upload_logs_to_s3(
            "server",
            c.container_id,
            c.secret_key,
            "Log message.",
        )

        assert response["status"] == HTTPStatus.OK


def test_bad_request(client):
    response = client.post("/logs", json=dict())
    assert response.status_code == HTTPStatus.BAD_REQUEST


def test_successful_request(client, monkeypatch):
    task = Object()

    monkeypatch.setattr(task, "id", "task_id")
    monkeypatch.setattr(upload_logs_to_s3, "apply_async", function(returns=task))

    response = client.post(
        "/logs",
        json=dict(
            sender="server",
            connection_id="myconnectionid123",
            identifier=0,
            secret_key="garbage!",
            logs="My cool message.",
        ),
    )

    assert response.status_code == HTTPStatus.ACCEPTED
