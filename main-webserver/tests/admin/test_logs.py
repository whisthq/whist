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
from app.models import db, ProtocolLog

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


@pytest.fixture
def log_entry():
    """Provide tests with a function that saves dummy log entries."""

    entries = []

    def _log_entry(container, connection_id):
        """Save a dummy log entry to the database.

        Arguments:
            container: The container from which this log entry originated.
            connection_id: Same as the argument to upload_logs_to_s3 with the same
                name.

        Returns:
            An instance of the ProtocolLog model.
        """

        log = ProtocolLog(
            user_id=container.user_id,
            connection_id=connection_id,
            bookmarked=False,
            timestamp=datetime.utcnow().timestamp() * 100000,
            version="master",
        )

        db.session.add(log)
        db.session.commit()

        return log

    yield _log_entry

    for entry in entries:
        db.session.delete(entry)

    db.session.commit()


def test_bad_sender():
    with pytest.raises(BadSenderError):
        upload_logs_to_s3("USPS", "master", "myconnectionid123", "x.x.x.x", 0, "", "Log message.")


def test_no_container():
    with pytest.raises(ContainerNotFoundError):
        upload_logs_to_s3("client", "master", "myconnectionid123", "x.x.x.x", 0, "", "Log message.")


def test_unauthorized(container):
    with container() as c:
        with pytest.raises(ContainerNotFoundError):
            upload_logs_to_s3(
                "client",
                "master",
                "myconnectionid123",
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
                "master",
                "myconnectionid123",
                c.ip,
                c.port_32262,
                c.secret_key,
                "Log message.",
            )


@pytest.mark.parametrize("sender", ("client", "server"))
def test_new_entry(container, sender, monkeypatch):
    is_client = sender == "client"
    connection_id = str(uuid.uuid4())

    monkeypatch.setattr(Resource.Object, "put", put_success, raising=False)
    monkeypatch.setattr(boto3, "resource", resource)

    with container() as c:
        upload_logs_to_s3(
            sender, "master", connection_id, c.ip, c.port_32262, c.secret_key, "Log message."
        )
        db.session.add(c)

        log = ProtocolLog.query.get(connection_id)

        assert log
        assert log.connection_id == connection_id
        assert log.user == c.user
        assert bool(log.client_logs) == is_client
        assert bool(log.server_logs) != is_client

        db.session.delete(log)
        db.session.commit()


@pytest.mark.parametrize("sender", ("client", "server"))
def test_update_entry(container, log_entry, sender, monkeypatch):
    connection_id = str(uuid.uuid4())

    monkeypatch.setattr(Resource.Object, "put", put_success, raising=False)
    monkeypatch.setattr(boto3, "resource", resource)

    with container() as c:
        dummy = log_entry(c, connection_id)

        assert not dummy.client_logs
        assert not dummy.server_logs

        upload_logs_to_s3(
            sender, "master", connection_id, c.ip, c.port_32262, c.secret_key, "Log message."
        )
        db.session.add(dummy)
        db.session.add(c)

        entry = ProtocolLog.query.get(connection_id)

        assert entry
        assert entry == dummy
        assert bool(entry.client_logs) != bool(entry.server_logs)

        with pytest.raises(AssertionError):
            upload_logs_to_s3(
                sender, "master", connection_id, c.ip, c.port_32262, c.secret_key, "Log message."
            )


def test_bad_request(client):
    response = client.post("/logs/insert", json=dict())

    assert response.status_code == 400


def test_successful(client, monkeypatch):
    monkeypatch.setattr(upload_logs_to_s3, "apply_async", apply_async)

    response = client.post(
        "/logs/insert",
        json=dict(
            sender="client",
            connection_id="myconnectionid123",
            identifier=0,
            secret_key="garbage!",
            logs="My cool message.",
        ),
    )

    assert response.status_code == 202
