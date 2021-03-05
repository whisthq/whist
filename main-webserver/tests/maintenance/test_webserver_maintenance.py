import time
import datetime
from datetime import timedelta, datetime as dt
import os

import pytest

from app.maintenance.maintenance_manager import (
    _REDIS_TASKS_KEY,
    _REDIS_UPDATE_KEY,
)
from app.celery.aws_ecs_creation import (
    _create_new_cluster,
    _assign_container,
)
from app.constants.http_codes import (
    ACCEPTED,
    SUCCESS,
    WEBSERVER_MAINTENANCE,
)
from app.constants.time import (
    SECONDS_IN_MINUTE,
    MINUTES_IN_HOUR,
    HOURS_IN_DAY,
)
from tests.helpers.general.progress import queryStatus
from tests.aws.test_assign import set_valid_subscription


def mock_create_cluster(*args, **kwargs):
    # need to import this here because this is no guarantee that the file
    # where this is executed has imported this
    from app.helpers.utils.general.logs import fractal_logger

    if hasattr(_create_new_cluster, "num_calls"):
        num_calls = getattr(_create_new_cluster, "num_calls") + 1
        setattr(_create_new_cluster, "num_calls", num_calls)
    else:
        setattr(_create_new_cluster, "num_calls", 1)

    celery_id = args[0].request.id
    fractal_logger.info(f"Running mocked version of task {celery_id}")

    time.sleep(1)


def mock_assign_container(*args, **kwargs):
    # need to import this here because this is no guarantee that the file
    # where this is executed has imported this
    from app.helpers.utils.general.logs import fractal_logger

    if hasattr(_assign_container, "num_calls"):
        num_calls = getattr(_assign_container, "num_calls") + 1
        setattr(_assign_container, "num_calls", num_calls)
    else:
        setattr(_assign_container, "num_calls", 1)

    celery_id = args[0].request.id
    fractal_logger.info(f"Running mocked version of task {celery_id}")

    time.sleep(1)


@pytest.fixture
def mock_endpoints(monkeypatch):
    """
    Mock _create_new_cluster and _assign_container. MUST be done before celery threads start.
    """
    # problematic:
    # /aws_container/create_cluster, /aws_container/assign_container, /container/assign
    monkeypatch.setattr(_create_new_cluster, "__code__", mock_create_cluster.__code__)
    monkeypatch.setattr(_assign_container, "__code__", mock_assign_container.__code__)


def try_problematic_endpoint(client, authorized, region_name: str, endpoint_type: str):
    assert endpoint_type in ["te_cc", "te_ac", "a_c"]

    resp = None
    if endpoint_type == "te_cc":
        # test_endpoint create_cluster
        create_cluster_body = dict(
            cluster_name="maintenance-test",
            instance_type="g3s.xlarge",
            region_name=region_name,
            max_size=1,
            min_size=0,
            username=authorized.user_id,
        )
        resp = client.post("/aws_container/create_cluster", json=create_cluster_body)

    elif endpoint_type == "te_ac":
        # TODO: make this a standardized var in tests
        deploy_env = "dev"
        if os.getenv("HEROKU_APP_NAME") == "fractal-prod-server":
            deploy_env = "prod"
        elif os.getenv("HEROKU_APP_NAME") == "fractal-staging-server":
            deploy_env = "staging"

        # test_endpoint assign_container
        assign_container_body = dict(
            username=authorized.user_id,
            cluster_name="maintenance-test",
            region_name=region_name,
            region=region_name,
            task_definition_arn="fractal-{}-browsers-chrome".format(deploy_env),
        )
        resp = client.post("/aws_container/assign_container", json=assign_container_body)

    elif endpoint_type == "a_c":
        # aws_container_assign endpoint used by client app
        assign_container_body = dict(
            username=authorized.user_id,
            app="Blender",
            region=region_name,
        )
        resp = client.post(
            "/container/assign",
            json=assign_container_body,
        )

    else:
        raise ValueError(f"Unrecognized endpoint_type {endpoint_type}")

    return resp


@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
def test_maintenance_mode_single_region(
    client,
    make_authorized_user,
    set_valid_subscription,
    mock_endpoints,
):
    """
    problematic task: create cluster or assign container, from /aws_container or /container/assign
    Test this maintenance mode access pattern:
    1. run a mocked problematic task that takes 1 second
    2. start maintenance mode, which should not succeed due to existing task but stop any new ones
    3. try a new problematic task, should fail out
    4. request maintenance mode again, this time succeeds due to first task being finished
    5. try another problematic task, should fail out because webserver in maintenance mode
    6. try a problematic task in a different region, which should succeed
    7. end maintenance mode
    8. try another problematic task in the original region, should succeed since maintenance over

    TODO:
    1. have multiple celery workers to concurrently run all problematic endpoints? Current strategy
    is to do a mix of them throughout test execution.
    """
    from app.maintenance.maintenance_manager import _REDIS_CONN

    # this is a free-trial user
    authorized = make_authorized_user(
        stripe_customer_id="random1234",
        created_timestamp=dt.now(datetime.timezone.utc).timestamp(),
    )
    set_valid_subscription(True)

    update_key = _REDIS_UPDATE_KEY.format(region_name="us-east-1")
    tasks_key = _REDIS_TASKS_KEY.format(region_name="us-east-1")

    # wipe db for a fresh start
    _REDIS_CONN.flushall()

    # -- Start the test -- #

    # start a task
    resp_start = try_problematic_endpoint(client, authorized, "us-east-1", "te_cc")
    assert resp_start.status_code == ACCEPTED

    # let celery actually start handling the task but not finish
    time.sleep(0.5)
    # update key should not exist yet.
    assert not _REDIS_CONN.exists(update_key)
    # task key should exist and have one item
    assert _REDIS_CONN.exists(tasks_key)
    tasks = _REDIS_CONN.lrange(tasks_key, 0, -1)
    assert len(tasks) == 1

    # start maintenance while existing task is running
    resp = client.post("/aws_container/start_update", json={"region_name": "us-east-1"})

    assert resp.status_code == SUCCESS
    assert resp.json["success"] is False  # False because a task is still running
    # the update key should exist now that someone started maintenance
    assert _REDIS_CONN.exists(update_key)

    # a new task should fail out, even though webserver is not in maintenance mode yet
    resp_fail = try_problematic_endpoint(client, authorized, "us-east-1", "te_cc")
    assert resp_fail.status_code == WEBSERVER_MAINTENANCE

    # poll original celery task finish
    task = queryStatus(client, resp_start, timeout=0.5)  # 0.5 minutes = 30 seconds
    assert task["status"] == 1, f"TASK: {task}"

    # _create_new_cluster (mock) should be called just once
    assert hasattr(_create_new_cluster, "num_calls")
    assert getattr(_create_new_cluster, "num_calls") == 1

    # now maintenance request should succeed
    resp = client.post("/aws_container/start_update", json={"region_name": "us-east-1"})
    assert resp.status_code == SUCCESS
    assert resp.json["success"] is True

    # new requests should still fail out
    resp = try_problematic_endpoint(client, authorized, "us-east-1", "te_cc")
    assert resp.status_code == WEBSERVER_MAINTENANCE
    resp = try_problematic_endpoint(client, authorized, "us-east-1", "te_ac")
    assert resp.status_code == WEBSERVER_MAINTENANCE
    resp = try_problematic_endpoint(client, authorized, "us-east-1", "a_c")
    assert resp.status_code == WEBSERVER_MAINTENANCE

    # test that tasks in other regions should proceed just fine
    resp = try_problematic_endpoint(client, authorized, "us-east-2", "te_ac")
    assert resp.status_code == ACCEPTED

    # now end maintenance
    resp = client.post("/aws_container/end_update", json={"region_name": "us-east-1"})
    assert resp.status_code == SUCCESS
    assert resp.json["success"] is True

    # update key should not exist anymore
    assert not _REDIS_CONN.exists(update_key)

    # tasks should work again
    resp_final = try_problematic_endpoint(client, authorized, "us-east-1", "a_c")
    assert resp_final.status_code == ACCEPTED

    task = queryStatus(client, resp_final, timeout=0.5)  # 0.1 minutes = 6 seconds
    assert task["status"] == 1, f"TASK: {task}"

    # _create_new_cluster (mock) should be called just once
    assert hasattr(_create_new_cluster, "num_calls")
    assert getattr(_create_new_cluster, "num_calls") == 1

    # # _assign_container (mock) should be called twice
    assert hasattr(_assign_container, "num_calls")
    assert getattr(_assign_container, "num_calls") == 2

    # clean up
    delattr(_create_new_cluster, "num_calls")
    delattr(_assign_container, "num_calls")


@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
def test_maintenance_mode_all_regions(
    client,
    make_authorized_user,
    set_valid_subscription,
    mock_endpoints,
):
    """
    See test_maintenance_mode_single_region. Only difference is that we try maintenance
    in all regions. This means the request in us-east-2 should also fail.
    """
    from app.maintenance.maintenance_manager import _REDIS_CONN

    # this is a free-trial user
    authorized = make_authorized_user(
        stripe_customer_id="random1234",
        created_timestamp=dt.now(datetime.timezone.utc).timestamp(),
    )
    set_valid_subscription(True)

    update_key = _REDIS_UPDATE_KEY.format(region_name="us-east-1")
    tasks_key = _REDIS_TASKS_KEY.format(region_name="us-east-1")

    # wipe db for a fresh start
    _REDIS_CONN.flushall()

    # -- Start the test -- #

    # start a task
    resp_start = try_problematic_endpoint(client, authorized, "us-east-1", "te_cc")
    assert resp_start.status_code == ACCEPTED

    # let celery actually start handling the task but not finish
    time.sleep(0.5)
    # update key should not exist yet.
    assert not _REDIS_CONN.exists(update_key)
    # task key should exist and have one item
    assert _REDIS_CONN.exists(tasks_key)
    tasks = _REDIS_CONN.lrange(tasks_key, 0, -1)
    assert len(tasks) == 1

    # start maintenance while existing task is running
    resp = client.post("/aws_container/start_update")

    assert resp.status_code == SUCCESS
    assert resp.json["success"] is False  # False because a task is still running
    # the update key should exist now that someone started maintenance
    assert _REDIS_CONN.exists(update_key)

    # a new task should fail out, even though webserver is not in maintenance mode yet
    resp_fail = try_problematic_endpoint(client, authorized, "us-east-1", "te_cc")
    assert resp_fail.status_code == WEBSERVER_MAINTENANCE

    # poll original celery task finish
    task = queryStatus(client, resp_start, timeout=0.5)  # 0.5 minutes = 30 seconds
    assert task["status"] == 1, f"TASK: {task}"

    # _create_new_cluster (mock) should be called just once
    assert hasattr(_create_new_cluster, "num_calls")
    assert getattr(_create_new_cluster, "num_calls") == 1

    # now maintenance request should succeed
    resp = client.post("/aws_container/start_update")
    assert resp.status_code == SUCCESS
    assert resp.json["success"] is True

    # new requests should still fail out
    resp = try_problematic_endpoint(client, authorized, "us-east-1", "te_cc")
    assert resp.status_code == WEBSERVER_MAINTENANCE
    resp = try_problematic_endpoint(client, authorized, "us-east-1", "te_ac")
    assert resp.status_code == WEBSERVER_MAINTENANCE
    resp = try_problematic_endpoint(client, authorized, "us-east-1", "a_c")
    assert resp.status_code == WEBSERVER_MAINTENANCE
    # test that tasks in other regions should also fail out
    resp = try_problematic_endpoint(client, authorized, "us-east-2", "te_ac")
    assert resp.status_code == WEBSERVER_MAINTENANCE

    # now end maintenance
    resp = client.post("/aws_container/end_update")
    assert resp.status_code == SUCCESS
    assert resp.json["success"] is True

    # update key should not exist anymore
    assert not _REDIS_CONN.exists(update_key)

    # tasks should work again
    resp_final = try_problematic_endpoint(client, authorized, "us-east-1", "a_c")
    assert resp_final.status_code == ACCEPTED

    task = queryStatus(client, resp_final, timeout=0.5)  # 0.1 minutes = 6 seconds
    assert task["status"] == 1, f"TASK: {task}"

    # _create_new_cluster (mock) should be called just once
    assert hasattr(_create_new_cluster, "num_calls")
    assert getattr(_create_new_cluster, "num_calls") == 1

    # # _assign_container (mock) should be called just once
    assert hasattr(_assign_container, "num_calls")
    assert getattr(_assign_container, "num_calls") == 1

    # clean up
    delattr(_create_new_cluster, "num_calls")
    delattr(_assign_container, "num_calls")
