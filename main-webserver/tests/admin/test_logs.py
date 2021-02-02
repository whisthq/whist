"""Tests for code required by the /admin/logs endpoint."""

import uuid

from datetime import datetime

import boto3
import pytest

from app.celery.aws_s3_modification import (
    BadSenderError,
    ContainerNotFoundError,
    upload_logs_to_s3,
)

from ..patches import apply_async


class Resource:
    class Object:
        def __init__(self, *args, **kwargs):
            pass


def resource(*args, **kwargs):
    return Resource()


def put_failure(*args, **kwargs):
    raise Exception


def put_success(*args, **kwargs):
    pass


def test_bad_sender():
    with pytest.raises(BadSenderError):
        upload_logs_to_s3("USPS", "x.x.x.x", 0, "", "Log message.")


def test_no_container():
    with pytest.raises(ContainerNotFoundError):
        upload_logs_to_s3("client", "x.x.x.x", 0, "", "Log message.")


def test_unauthorized(container):
    with container() as c:
        with pytest.raises(ContainerNotFoundError):
            upload_logs_to_s3(
                "client",
                c.ip,
                c.port_32262,
                "garbage!",
                "Log message.",
            )


def test_s3_failure(container, monkeypatch):
    monkeypatch.setattr(Resource.Object, "put", put_failure, raising=False)
    monkeypatch.setattr(boto3, "resource", resource)

    with container() as c:
        with pytest.raises(Exception):
            upload_logs_to_s3(
                "client",
                c.ip,
                c.port_32262,
                c.secret_key,
                "Log message.",
            )


def test_s3_success(container, monkeypatch):
    monkeypatch.setattr(Resource.Object, "put", put_success, raising=False)
    monkeypatch.setattr(boto3, "resource", resource)

    with container() as c:
        response = upload_logs_to_s3(
            "server",
            c.ip,
            c.port_32262,
            c.secret_key,
            "Log message.",
        )
        assert response["status"] == 200


def test_bad_request(client):
    response = client.post("/logs", json=dict())
    assert response.status_code == 400


def test_successful_request(client, monkeypatch):
    monkeypatch.setattr(upload_logs_to_s3, "apply_async", apply_async)

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

    assert response.status_code == 202
