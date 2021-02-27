import time

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
from app.helpers.utils.general.logs import fractal_log
from tests.helpers.general.progress import queryStatus
from tests.aws.test_assign import set_valid_subscription


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


@pytest.fixture
def mock_endpoints(monkeypatch):
    """
    Mock _create_new_cluster and _assign_container. MUST be done before celery threads start.
    """
    # problematic:
    # /aws_container/create_cluster, /aws_container/assign_container, /container/assign
    monkeypatch.setattr(_create_new_cluster, "__code__", mock_create_cluster.__code__)
    monkeypatch.setattr(_assign_container, "__code__", mock_assign_container.__code__)


@pytest.fixture
def try_start_maintenance(client):
    """Try to put the web server into maintenance mode.

    Args:
        region: The name of the region for which we are trying to start maintenance mode as a
            string or None if we are entering maintenance mode in all regions.

    Returns:
        An HTTP response object.
    """

    def _start_maintenance(region=None):
        kwargs = {}

        if region is not None:
            kwargs["json"] = {"region_name": region}

        return client.post("/aws_container/start_update", **kwargs)

    return _start_maintenance


@pytest.fixture
def try_end_maintenance(client):
    """Try to take the web server out of maintenance mode.

    Args:
        region: The name of the region for which we are stopping maintenance mode as a string or
            None if we are trying to leave maintenance mode in all regions.

    Returns:
        An HTTP response object.
    """

    def _end_maintenance(region=None):
        kwargs = {}

        if region is not None:
            kwargs["json"] = {"region_name": region}

        return client.post("/aws_container/end_update", **kwargs)

    return _end_maintenance


@pytest.fixture
def try_problematic_endpoint(client, request):
    """Send a request to a maintenance-mode protected endpoint.

    Args:
        endpoint_type: Satan himself as a string.
        region: The value of the region_name POST key to include in the request as a string.
        username: The user ID of the user on behalf of whom to make the request as a string.

    Returns:
        An HTTP response object.
    """

    def _problematic_request(endpoint_type, region, username):
        assert endpoint_type in ["te_cc", "te_ac", "a_c"]

        resp = None
        if endpoint_type == "te_cc":
            # test_endpoint create_cluster
            create_cluster_body = dict(
                cluster_name="maintenance-test",
                instance_type="g3s.xlarge",
                region_name=region,
                max_size=1,
                min_size=0,
                username=username,
            )
            resp = client.post("/aws_container/create_cluster", json=create_cluster_body)

        elif endpoint_type == "te_ac":
            deployment_stage = request.getfixturevalue("deployment_stage")

            # test_endpoint assign_container
            assign_container_body = dict(
                username=username,
                cluster_name="maintenance-test",
                region_name=region,
                task_definition_arn=f"fractal-{deployment_stage}-browsers-chrome",
            )
            resp = client.post("/aws_container/assign_container", json=assign_container_body)

        elif endpoint_type == "a_c":
            # aws_container_assign endpoint used by client app
            assign_container_body = dict(
                username=username,
                app="Blender",
                region=region,
            )
            resp = client.post(
                "/container/assign",
                json=assign_container_body,
            )

        else:
            raise ValueError(f"Unrecognized endpoint_type {endpoint_type}")

        return resp

    return _problematic_request


@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
@pytest.mark.usefixtures("mock_endpoints")
def test_maintenance_mode_single_region(
    client,
    make_authorized_user,
    set_valid_subscription,
    try_end_maintenance,
    try_problematic_endpoint,
    try_start_maintenance,
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
    user = make_authorized_user(stripe_customer_id="random1234")
    set_valid_subscription(True)

    update_key = _REDIS_UPDATE_KEY.format(region_name="us-east-1")
    tasks_key = _REDIS_TASKS_KEY.format(region_name="us-east-1")

    # wipe db for a fresh start
    _REDIS_CONN.flushall()

    # -- Start the test -- #

    # start a task
    resp_start = try_problematic_endpoint("te_cc", "us-east-1", user.user_id)
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
    resp = try_start_maintenance("us-east-1")
    assert resp.status_code == SUCCESS
    assert resp.json["success"] is False  # False because a task is still running
    # the update key should exist now that someone started maintenance
    assert _REDIS_CONN.exists(update_key)

    # a new task should fail out, even though webserver is not in maintenance mode yet
    resp_fail = try_problematic_endpoint("te_cc", "us-east-1", user.user_id)
    assert resp_fail.status_code == WEBSERVER_MAINTENANCE

    # poll original celery task finish
    task = queryStatus(client, resp_start, timeout=0.5)  # 0.5 minutes = 30 seconds
    assert task["status"] == 1, f"TASK: {task}"

    # _create_new_cluster (mock) should be called just once
    assert hasattr(_create_new_cluster, "num_calls")
    assert getattr(_create_new_cluster, "num_calls") == 1

    # now maintenance request should succeed
    resp = try_start_maintenance("us-east-1")
    assert resp.status_code == SUCCESS
    assert resp.json["success"] is True

    # new requests should still fail out
    resp = try_problematic_endpoint("te_cc", "us-east-1", user.user_id)
    assert resp.status_code == WEBSERVER_MAINTENANCE
    resp = try_problematic_endpoint("te_ac", "us-east-1", user.user_id)
    assert resp.status_code == WEBSERVER_MAINTENANCE
    resp = try_problematic_endpoint("a_c", "us-east-1", user.user_id)
    assert resp.status_code == WEBSERVER_MAINTENANCE

    # test that tasks in other regions should proceed just fine
    resp = try_problematic_endpoint("te_ac", "us-east-2", user.user_id)
    assert resp.status_code == ACCEPTED

    # now end maintenance
    resp = try_end_maintenance("us-east-1")
    assert resp.status_code == SUCCESS
    assert resp.json["success"] is True

    # update key should not exist anymore
    assert not _REDIS_CONN.exists(update_key)

    # tasks should work again
    resp_final = try_problematic_endpoint("a_c", "us-east-1", user.user_id)
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
@pytest.mark.usefixtures("mock_endpoints")
def test_maintenance_mode_all_regions(
    client,
    make_authorized_user,
    set_valid_subscription,
    try_end_maintenance,
    try_problematic_endpoint,
    try_start_maintenance,
):
    """
    See test_maintenance_mode_single_region. Only difference is that we try maintenance
    in all regions. This means the request in us-east-2 should also fail.
    """
    from app.maintenance.maintenance_manager import _REDIS_CONN

    # this is a free-trial user
    user = make_authorized_user(stripe_customer_id="random1234")
    set_valid_subscription(True)

    update_key = _REDIS_UPDATE_KEY.format(region_name="us-east-1")
    tasks_key = _REDIS_TASKS_KEY.format(region_name="us-east-1")

    # wipe db for a fresh start
    _REDIS_CONN.flushall()

    # -- Start the test -- #

    # start a task
    resp_start = try_problematic_endpoint("te_cc", "us-east-1", user.user_id)
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
    resp = try_start_maintenance()

    assert resp.status_code == SUCCESS
    assert resp.json["success"] is False  # False because a task is still running
    # the update key should exist now that someone started maintenance
    assert _REDIS_CONN.exists(update_key)

    # a new task should fail out, even though webserver is not in maintenance mode yet
    resp_fail = try_problematic_endpoint("te_cc", "us-east-1", user.user_id)
    assert resp_fail.status_code == WEBSERVER_MAINTENANCE

    # poll original celery task finish
    task = queryStatus(client, resp_start, timeout=0.5)  # 0.5 minutes = 30 seconds
    assert task["status"] == 1, f"TASK: {task}"

    # _create_new_cluster (mock) should be called just once
    assert hasattr(_create_new_cluster, "num_calls")
    assert getattr(_create_new_cluster, "num_calls") == 1

    # now maintenance request should succeed
    resp = try_start_maintenance()
    assert resp.status_code == SUCCESS
    assert resp.json["success"] is True

    # new requests should still fail out
    resp = try_problematic_endpoint("te_cc", "us-east-1", user.user_id)
    assert resp.status_code == WEBSERVER_MAINTENANCE
    resp = try_problematic_endpoint("te_ac", "us-east-1", user.user_id)
    assert resp.status_code == WEBSERVER_MAINTENANCE
    resp = try_problematic_endpoint("a_c", "us-east-1", user.user_id)
    assert resp.status_code == WEBSERVER_MAINTENANCE
    # test that tasks in other regions should also fail out
    resp = try_problematic_endpoint("te_ac", "us-east-2", user.user_id)
    assert resp.status_code == WEBSERVER_MAINTENANCE

    # now end maintenance
    resp = try_end_maintenance()
    assert resp.status_code == SUCCESS
    assert resp.json["success"] is True

    # update key should not exist anymore
    assert not _REDIS_CONN.exists(update_key)

    # tasks should work again
    resp_final = try_problematic_endpoint("a_c", "us-east-1", user.user_id)
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
