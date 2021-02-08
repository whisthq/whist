import time
import copy
from functools import wraps
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
    """
    FAKKKKKEE
    """
    if hasattr(_create_new_cluster, "num_calls"):
        num_calls = getattr(_create_new_cluster, "num_calls") + 1
        setattr(_create_new_cluster, "num_calls", num_calls)
    else:
        setattr(_create_new_cluster, "num_calls", 1)

    celery_id = args[0].request.id
    fractal_log("mock_celery_endpoint", None, f"HERE WITH TASK {celery_id}")

    time.sleep(1)


def mock_endpoints(monkeypatch):
    # problematic:
    # /aws_container/create_cluster, /aws_container/assign_container, /container/assign
    # monkeypatch.setattr("app.celery.aws_ecs_creation._create_new_cluster", mock_celery_endpoint)
    _create_new_cluster.__doc__ = mock_celery_endpoint.__doc__
    _create_new_cluster.__code__ = mock_celery_endpoint.__code__
    # raise ValueError(_create_new_cluster.__doc__)
    # monkeypatch.setattr("app.celery.aws_ecs_creation._assign_container", mock_celery_endpoint)


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


def try_problematic_endpoint(client, admin, region_name: str):
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

    fractal_log(
        "maintenace_test",
        None,
        f"BEFORE DOCSTRING: {_create_new_cluster.__doc__}",
    )

    # cc = create_cluster, te = test_endpoint
    resp = client.post(
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

    return resp.status_code # resp_ac_te.status_code, ]# resp_ac_aca.status_code]


@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
@pytest.mark.usefixtures("admin")
def test_maintenance_mode(client, admin, monkeypatch):
    """
    Test the following maintenance mode access pattern:
    1. mocked problematic task that takes 1 second
    2. start maintenance mode, which should not succeed due to existing task but stop any new ones
    3. try a new problematic task, should fail out
    4. request maintenance mode again, this time succeeds due to first task being finished
    5. try another problematic task, should fail out because webserver in maintenance mode
    6. try a problematic task in a different region, which should succeed
    7. end maintenance mode
    8. try another problematic task in the original region, should succeed since maintenance mode over

    TODO: have multiple celery workers to concurrently run all problematic endpoints? Current strategy is
    to do a mix of them throughout test execution.
    """
    from app import redis_conn
    update_key = _REDIS_UPDATE_KEY.format(region_name="us-east-1")
    tasks_key = _REDIS_TASKS_KEY.format(region_name="us-east-1")

    # wipe these for a fresh start
    redis_conn.delete(update_key)
    redis_conn.delete(tasks_key)

    mock_endpoints(monkeypatch)

    # -- Start the test -- #

    # start a task
    p_code = try_problematic_endpoint(client, admin, "us-east-1")
    assert p_code == ACCEPTED

    # let celery actually start handling the task but not finish
    time.sleep(0.5)
    # update key should not exist yet.
    assert not redis_conn.exists(update_key)
    # task key should exist and have one item
    assert redis_conn.exists(tasks_key)
    tasks = redis_conn.lrange(tasks_key, 0, -1)
    assert len(tasks) == 1

    # start maintenance while existing task is running
    code, response = try_start_maintenance(client, "us-east-1")
    assert code == ACCEPTED
    assert response["success"] is False # False because a task is still running
    # the update key should exist now that someone started maintenance
    assert redis_conn.exists(update_key)
    
    # a new task should fail out, even though webserver is not in maintenance mode yet
    p_code = try_problematic_endpoint(client, admin, "us-east-1")
    assert p_code == WEBSERVER_MAINTENANCE

    # let original celery task finish
    time.sleep(1)

    # # mock_celery_endpoint should be called just once
    assert hasattr(_create_new_cluster, "num_calls")
    assert getattr(_create_new_cluster, "num_calls") == 1

    # now maintenance request should succeed
    code, response = try_start_maintenance(client, "us-east-1")
    assert code == ACCEPTED
    assert response["success"] is True

    # new requests should still fail out
    p_code = try_problematic_endpoint(client, admin, "us-east-1")
    assert p_code == WEBSERVER_MAINTENANCE

    # test that tasks in other regions should proceed just fine
    p_code = try_problematic_endpoint(client, admin, "us-east-2")
    assert p_code == ACCEPTED

    # now end maintenance
    code, response = try_end_maintenance(client, "us-east-1")
    assert code == ACCEPTED
    assert response["success"] is True

    # update key should not exist anymore
    assert not redis_conn.exists(update_key)

    # tasks should work again
    p_code = try_problematic_endpoint(client, admin, "us-east-1")
    assert p_code == ACCEPTED

