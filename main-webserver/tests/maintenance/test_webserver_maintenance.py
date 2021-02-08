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
from app.constants.http_codes import (
    ACCEPTED,
    WEBSERVER_MAINTENANCE,
)
from app.helpers.utils.general.logs import fractal_log


def mock_create_cluster(*args, **kwargs):
    if hasattr(_create_new_cluster, "num_calls"):
        num_calls = getattr(_create_new_cluster, "num_calls") + 1
        setattr(_create_new_cluster, "num_calls", num_calls)
    else:
        setattr(_create_new_cluster, "num_calls", 1)

    celery_id = args[0].request.id
    fractal_log("mock_celery_endpoint", None, f"HERE WITH TASK {celery_id}")

    time.sleep(1)


def mock_assign_container(*args, **kwargs):
    if hasattr(_assign_container, "num_calls"):
        num_calls = getattr(_assign_container, "num_calls") + 1
        setattr(_assign_container, "num_calls", num_calls)
    else:
        setattr(_assign_container, "num_calls", 1)

    celery_id = args[0].request.id
    fractal_log("mock_celery_endpoint", None, f"HERE WITH TASK {celery_id}")

    time.sleep(1)


def mock_endpoints():
    # problematic:
    # /aws_container/create_cluster, /aws_container/assign_container, /container/assign
    _create_new_cluster.__doc__ = mock_create_cluster.__doc__
    _create_new_cluster.__code__ = mock_create_cluster.__code__

    _assign_container.__doc__ = mock_assign_container.__doc__
    _assign_container.__code__ = mock_assign_container.__code__


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


def try_problematic_endpoint(client, admin, region_name: str, endpoint_type: str):
    assert endpoint_type in ["te_cc", "te_ac"]

    resp = None
    if endpoint_type == "te_cc":
        # test_endpoint create_cluster
        create_cluster_body = dict(
            cluster_name="maintenance-test",
            instance_type="g3s.xlarge",
            region_name=region_name,
            max_size=1,
            min_size=0,
            username=admin.user_id,
        )
        resp = client.post(
            "/aws_container/create_cluster",
            json=create_cluster_body
        )

    elif endpoint_type == "te_ac":
        # test_endpoint assign_container
        assign_container_body = dict(
            username=admin.user_id,
            cluster_name="maintenance-test",
            region_name=region_name,
            region=region_name,
            task_definition_arn="fractal-browsers-chrome",
        )
        resp = client.post(
            "/aws_container/assign_container",
            json=assign_container_body
        )

    else:
        raise ValueError(f"Unrecognized endpoint_type {endpoint_type}")

    return resp.status_code


@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
@pytest.mark.usefixtures("admin")
def test_maintenance_mode(client, admin):
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

    TODO:
    1. have multiple celery workers to concurrently run all problematic endpoints? Current strategy is
    to do a mix of them throughout test execution.
    2. We need to add fixtures so we can hit /container/assign and make sure it is locked down
    """
    from app import redis_conn
    update_key = _REDIS_UPDATE_KEY.format(region_name="us-east-1")
    tasks_key = _REDIS_TASKS_KEY.format(region_name="us-east-1")

    # wipe these for a fresh start
    redis_conn.delete(update_key)
    redis_conn.delete(tasks_key)

    mock_endpoints()

    # -- Start the test -- #

    # start a task
    code = try_problematic_endpoint(client, admin, "us-east-1", "te_cc")
    assert code == ACCEPTED

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
    code = try_problematic_endpoint(client, admin, "us-east-1", "te_cc")
    assert code == WEBSERVER_MAINTENANCE

    # let original celery task finish
    time.sleep(1)

    # _create_new_cluster (mock) should be called just once
    assert hasattr(_create_new_cluster, "num_calls")
    assert getattr(_create_new_cluster, "num_calls") == 1

    # now maintenance request should succeed
    code, response = try_start_maintenance(client, "us-east-1")
    assert code == ACCEPTED
    assert response["success"] is True

    # new requests should still fail out
    code = try_problematic_endpoint(client, admin, "us-east-1", "te_cc")
    assert code == WEBSERVER_MAINTENANCE
    code = try_problematic_endpoint(client, admin, "us-east-1", "te_ac")
    assert code == WEBSERVER_MAINTENANCE
    code = try_problematic_endpoint(client, admin, "us-east-1", "te_ac")
    assert code == WEBSERVER_MAINTENANCE

    # test that tasks in other regions should proceed just fine
    code = try_problematic_endpoint(client, admin, "us-east-2", "te_ac")
    assert code == ACCEPTED

    # now end maintenance
    code, response = try_end_maintenance(client, "us-east-1")
    assert code == ACCEPTED
    assert response["success"] is True

    # update key should not exist anymore
    assert not redis_conn.exists(update_key)

    # tasks should work again
    code = try_problematic_endpoint(client, admin, "us-east-1", "te_ac")
    assert code == ACCEPTED

    # let task finish
    time.sleep(1.5)

    # _create_new_cluster (mock) should be called just once
    assert hasattr(_create_new_cluster, "num_calls")
    assert getattr(_create_new_cluster, "num_calls") == 1

    # _assign_container (mock) should be called just once
    assert hasattr(_assign_container, "num_calls")
    assert getattr(_assign_container, "num_calls") == 2

