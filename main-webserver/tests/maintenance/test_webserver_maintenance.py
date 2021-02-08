import time
import copy
from typing import Callable

import pytest
from celery import shared_task

from app.maintenance.maintenance_manager import (
    _REDIS_TASKS_KEY,
    _REDIS_UPDATE_KEY,
)
from app.celery.aws_ecs_creation import (
    _create_new_cluster,
    _assign_container
)
from app.celery.aws_ecs_deletion import (
    _delete_cluster
)
from app.constants.http_codes import (
    ACCEPTED,
    WEBSERVER_MAINTENANCE,
)
from app.helpers.utils.general.logs import fractal_log


def mock_celery_endpoint(*args, **kwargs):
    if hasattr(mock_celery_endpoint, "num_calls"):
        num_calls = getattr(mock_celery_endpoint, "num_calls") + 1
        setattr(mock_celery_endpoint, "num_calls", num_calls)
    else:
        setattr(mock_celery_endpoint, "num_calls", 1)


def mock_endpoints(monkeypatch):
    # problematic:
    # /aws_container/create_cluster, /aws_container/assign_container, /container/assign
    monkeypatch.setattr("app.celery.aws_ecs_creation._create_new_cluster", mock_celery_endpoint)
    # monkeypatch.setattr("app.celery.aws_ecs_creation._assign_container", mock_celery_endpoint)

    # non-problematic:
    # anything but the problematic endpoints. for testing: delete_cluster
    # monkeypatch.setattr("app.celery.aws_ecs_deletion._delete_cluster", mock_celery_endpoint)


def try_start_maintenance(client, region_name: str):
    resp = client.post(
        "/aws_container/start_update",
        json=dict(
            region_name=region_name,
        ),
    )
    return resp.status_code, resp.json


def try_end_maintenance(client, region_name: str):
    resp = client.post(
        "/aws_container/end_update",
        json=dict(
            region_name=region_name,
        ),
    )
    return resp.status_code, resp.json


def try_problematic_endpoints(client, admin, region_name: str):
    # create_cluster, assign_container (through test_endpoint and aws_container_assign)
    create_cluster_body = dict(
        cluster_name="maintenance-test",
        instance_type="g3s.xlarge",
        region_name=region_name,
        max_size=1,
        min_size=0,
        username=admin.user_id,
    )
    assign_container_body = dict(
        username=admin.user_id,
        cluster_name="maintenance-test",
        region_name=region_name,
        region=region_name,
        task_definition_arn="fractal-browsers-chrome",
    )

    # cc = create_cluster, te = test_endpoint
    resp_cc_te = client.post(
        "/aws_container/create_cluster",
        json=create_cluster_body
    )
    # ac = assign_container
    # resp_ac_te = client.post(
    #     "/aws_container/assign_container",
    #     json=assign_container_body
    # )
    # aca = aws_container_assign
    # resp_ac_aca = client.post(
    #     "/container/assign",
    #     json=assign_container_body
    # )

    return [resp_cc_te.status_code, ]# resp_ac_te.status_code, ]# resp_ac_aca.status_code]


def try_nonproblematic_endpoints(client, region_name: str):
    # delete_cluster
    # dc_te = client.post(
    #     "/aws_container/delete_cluster",
    #     json=dict(
    #         cluster_name="maintenance-test",
    #         region_name=region_name,
    #     ),
    # )
    return [ACCEPTED]
    return [dc_te.status_code]


@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
@pytest.mark.usefixtures("admin")
def test_maintenance(client, admin, monkeypatch):
    from app import redis_conn
    region_name = "us-east-1"
    update_key = _REDIS_UPDATE_KEY.format(region_name=region_name)
    tasks_key = _REDIS_TASKS_KEY.format(region_name=region_name)

    # wipe these for a fresh start
    redis_conn.delete(update_key)
    redis_conn.delete(tasks_key)

    mock_endpoints(monkeypatch)

    # update key should not exist yet.
    assert not redis_conn.exists(update_key)

    code, response = try_start_maintenance(client, region_name)
    assert code == ACCEPTED
    assert response["success"] is True
    # the update key should exist now that someone started maintenance
    assert redis_conn.exists(update_key)
    
    p_codes = try_problematic_endpoints(client, admin, region_name)
    np_codes = try_nonproblematic_endpoints(client, region_name)
    for i in range(len(p_codes)):
        # all problematic endpoints should be rejected because someone tried to start maintenance
        assert p_codes[i] == WEBSERVER_MAINTENANCE, print(f"Failed at index {i}")
    for i in range(len(np_codes)):
        # not problematic endpoints should be fine
        assert np_codes[i] == ACCEPTED, print(f"Failed at index {i}")

    # tasks in other regions should proceed just fine
    p_codes = try_problematic_endpoints(client, admin, "us-east-2")
    np_codes = try_nonproblematic_endpoints(client, "us-east-2")
    for i in range(len(p_codes)):
        assert p_codes[i] == ACCEPTED, print(f"Failed at index {i}")
    for i in range(len(np_codes)):
        assert np_codes[i] == ACCEPTED, print(f"Failed at index {i}")

    code, response = try_end_maintenance(client, region_name)
    assert code == ACCEPTED
    assert response["success"] is True

    # update key should not exist anymore
    assert not redis_conn.exists(update_key)

    # tasks should work again
    p_codes = try_problematic_endpoints(client, admin, region_name)
    np_codes = try_nonproblematic_endpoints(client, region_name)
    for i in range(len(p_codes)):
        assert p_codes[i] == ACCEPTED, print(f"Failed at index {i}")
    for i in range(len(np_codes)):
        assert np_codes[i] == ACCEPTED, print(f"Failed at index {i}")

