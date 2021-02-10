import time
import copy
import decorator
from typing import Callable

import pytest
from celery import shared_task

from app.maintenance.maintenance_manager import (
    _REDIS_TASKS_KEY,
    _REDIS_UPDATE_KEY,
)
from app.celery.aws_ecs_creation import _create_new_cluster, _assign_container
from app.constants.http_codes import (
    ACCEPTED,
    WEBSERVER_MAINTENANCE,
)
from app.helpers.utils.general.logs import fractal_log
from tests.helpers.general.progress import queryStatus


def mock_create_cluster(*args, **kwargs):
    if hasattr(_create_new_cluster, "num_calls"):
        num_calls = getattr(_create_new_cluster, "num_calls") + 1
        setattr(_create_new_cluster, "num_calls", num_calls)
    else:
        setattr(_create_new_cluster, "num_calls", 1)

    celery_id = args[0].request.id
    fractal_log("mock_create_cluster", None, f"Running mocked version of task {celery_id}")

    time.sleep(1)


def mock_assign_container(*args, **kwargs):
    if hasattr(_assign_container, "num_calls"):
        num_calls = getattr(_assign_container, "num_calls") + 1
        setattr(_assign_container, "num_calls", num_calls)
    else:
        setattr(_assign_container, "num_calls", 1)

    celery_id = args[0].request.id
    fractal_log("mock_assign_container", None, f"Running mocked version of task {celery_id}")

    time.sleep(1)


def mock_endpoints():
    """ Manual mocking because this needs to be done before celery threads start. """
    # problematic:
    # /aws_container/create_cluster, /aws_container/assign_container, /container/assign
    create_new_cluster_code = copy.deepcopy(_create_new_cluster.__code__)
    _create_new_cluster.__code__ = mock_create_cluster.__code__

    assign_container_code = copy.deepcopy(_assign_container.__code__)
    _assign_container.__code__ = mock_assign_container.__code__

    return create_new_cluster_code, assign_container_code


def unmock_endpoints(create_new_cluster_code, assign_container_code):
    """ Undo the mocking of create cluster and assign container """
    _create_new_cluster.__code__ = create_new_cluster_code
    _assign_container.__code__ = assign_container_code


def try_start_maintenance(client, region_name: str):
    resp = client.post(
        "/aws_container/start_update",
        json=dict(
            region_name=region_name,
        ),
    )
    return resp


def try_end_maintenance(client, region_name: str):
    resp = client.post(
        "/aws_container/end_update",
        json=dict(
            region_name=region_name,
        ),
    )
    return resp


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
        resp = client.post("/aws_container/create_cluster", json=create_cluster_body)

    elif endpoint_type == "te_ac":
        # test_endpoint assign_container
        assign_container_body = dict(
            username=admin.user_id,
            cluster_name="maintenance-test",
            region_name=region_name,
            region=region_name,
            task_definition_arn="fractal-browsers-chrome",
        )
        resp = client.post("/aws_container/assign_container", json=assign_container_body)

    else:
        raise ValueError(f"Unrecognized endpoint_type {endpoint_type}")

    return resp


def custom_monkeypatch(func):
    """
    This decorator is needed to patch before celery fixtures are applied.
    The decorated test can only be run once.
    """
    # mock right away to stop race-condition with celery threads. mocking inside
    # wrapper() would cause problems.
    create_new_cluster_code, assign_container_code = mock_endpoints()
    def wrapper(func, *args, **kwargs):
        nonlocal create_new_cluster_code, assign_container_code
        to_ret = func(*args, **kwargs)
        unmock_endpoints(create_new_cluster_code, assign_container_code)
        return to_ret

    # using functools.wraps does not work. This post helped:
    # https://stackoverflow.com/questions/19614658/how-do-i-make-pytest-fixtures-work-with-decorated-functions
    return decorator.decorator(wrapper, func)


@custom_monkeypatch
@pytest.mark.usefixtures("admin")
@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
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

    # mock endpoints (will call again to unmock)
    # mock_endpoints()

    # -- Start the test -- #

    # start a task
    resp_start = try_problematic_endpoint(client, admin, "us-east-1", "te_cc")
    assert resp_start.status_code == ACCEPTED

    # let celery actually start handling the task but not finish
    time.sleep(0.5)
    # update key should not exist yet.
    assert not redis_conn.exists(update_key)
    # task key should exist and have one item
    assert redis_conn.exists(tasks_key)
    tasks = redis_conn.lrange(tasks_key, 0, -1)
    assert len(tasks) == 1

    # start maintenance while existing task is running
    resp = try_start_maintenance(client, "us-east-1")
    
    assert resp.status_code == ACCEPTED
    assert resp.json["success"] is False  # False because a task is still running
    # the update key should exist now that someone started maintenance
    assert redis_conn.exists(update_key)

    # a new task should fail out, even though webserver is not in maintenance mode yet
    resp_fail = try_problematic_endpoint(client, admin, "us-east-1", "te_cc")
    assert resp_fail.status_code == WEBSERVER_MAINTENANCE

    # poll original celery task finish
    task = queryStatus(client, resp_start, timeout=0.5) # 0.5 minutes = 30 seconds
    assert task["status"] == 1, f"TASK: {task}"

    # _create_new_cluster (mock) should be called just once
    assert hasattr(_create_new_cluster, "num_calls")
    assert getattr(_create_new_cluster, "num_calls") == 1

    # now maintenance request should succeed
    resp = try_start_maintenance(client, "us-east-1")
    assert resp.status_code == ACCEPTED
    assert resp.json["success"] is True

    # new requests should still fail out
    resp = try_problematic_endpoint(client, admin, "us-east-1", "te_cc")
    assert resp.status_code == WEBSERVER_MAINTENANCE
    resp = try_problematic_endpoint(client, admin, "us-east-1", "te_ac")
    assert resp.status_code == WEBSERVER_MAINTENANCE

    # test that tasks in other regions should proceed just fine
    resp = try_problematic_endpoint(client, admin, "us-east-2", "te_ac")
    assert resp.status_code == ACCEPTED

    # now end maintenance
    resp = try_end_maintenance(client, "us-east-1")
    assert resp.status_code == ACCEPTED
    assert resp.json["success"] is True

    # update key should not exist anymore
    assert not redis_conn.exists(update_key)

    # tasks should work again
    resp_final = try_problematic_endpoint(client, admin, "us-east-1", "te_ac")
    assert resp.status_code == ACCEPTED

    task = queryStatus(client, resp_final, timeout=0.5) # 0.1 minutes = 6 seconds
    assert task["status"] == 1, f"TASK: {task}"

    # _create_new_cluster (mock) should be called just once
    assert hasattr(_create_new_cluster, "num_calls")
    assert getattr(_create_new_cluster, "num_calls") == 1

    # # _assign_container (mock) should be called just once
    assert hasattr(_assign_container, "num_calls")
    assert getattr(_assign_container, "num_calls") == 2

