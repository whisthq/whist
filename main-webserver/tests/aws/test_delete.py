"""Tests for the /container/delete endpoint."""

import uuid

from http import HTTPStatus
import datetime

import pytest

from app.celery.aws_ecs_deletion import delete_container
from app.helpers.utils.general.time import date_to_unix, get_today
from app.celery.aws_ecs_modification import check_and_cleanup_outdated_tasks
from app.models import db

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


def test_unauthorized(container, monkeypatch):
    c = container()

    with pytest.raises(Exception):
        delete_container(c.container_id, "garbage!")


@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
def test_autodelete_nonpinging_containers(cluster, bulk_container, monkeypatch):
    bad_ping_time = date_to_unix(get_today() + datetime.timedelta(seconds=-91))
    good_ping_time = date_to_unix(get_today() + datetime.timedelta(seconds=-10))

    c_bad = bulk_container(cluster_name=cluster.cluster, location=cluster.location)
    c_bad.last_pinged = bad_ping_time

    c_good = bulk_container(cluster_name=cluster.cluster, location=cluster.location)
    c_good.last_pinged = good_ping_time

    # make sure a container with NULL last_pinged is ignored
    c_prewarmed = bulk_container(cluster_name=cluster.cluster, location=cluster.location)
    c_prewarmed.last_pinged = None

    # list of containers that mocked_delete_container is called by
    delete_list = []
    expected_delete_list = [c_bad.container_id]

    db.session.commit()

    def mocked_delete_container(container_id, aes_key):
        delete_list.append(container_id)

    # make sure ensure_container_exists works passes
    monkeypatch.setattr(delete_container, "delay", mocked_delete_container)

    # call cleanup routine
    check_and_cleanup_outdated_tasks(cluster=cluster.cluster, region_name=cluster.location)

    assert set(delete_list) == set(expected_delete_list)
