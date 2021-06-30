import uuid

import pytest

from flask_jwt_extended import create_access_token

from app.celery import aws_ecs_creation
from app.celery import aws_ecs_modification
from app.celery.aws_ecs_creation import prewarm_new_container
from app.celery.aws_ecs_deletion import delete_cluster
from app.celery.aws_ecs_modification import manual_scale_cluster, update_cluster
from app.helpers.utils.aws.base_ecs_client import ECSClient
from app.helpers.utils.aws.aws_resource_integrity import ensure_container_exists
from app.helpers.utils.general.logs import fractal_logger
from app.helpers.utils.general.sql_commands import fractal_sql_commit
from app.models import (
    db,
    ClusterInfo,
    UserContainer,
    RegionToAmi,
    SupportedAppImages,
)
from app.constants.http_codes import (
    SUCCESS,
    BAD_REQUEST,
)
from tests.conftest import user
from tests.helpers.general.progress import queryStatus
from tests.patches import function

GENERIC_UBUNTU_SERVER_2004_LTS_AMI = "ami-0885b1f6bd170450c"
module_user = pytest.fixture(scope="module")(user)


@pytest.fixture(scope="module")
def ecs_data(app):
    # current_app.config needed for generate_name
    with app.app_context():
        pytest.cluster_name = ECSClient.generate_name(starter_name="cluster", test_prefix=True)
        pytest.container_name = None


@pytest.mark.container_serial
@pytest.mark.usefixtures("celery_worker")
@pytest.mark.usefixtures("ecs_data")
def test_create_cluster(client, module_user):
    cluster_name = pytest.cluster_name

    client.login(module_user, admin=True)
    fractal_logger.info("Starting to create cluster {}".format(cluster_name))

    resp = client.post(
        "/aws_container/create_cluster",
        json=dict(
            cluster_name=cluster_name,
            instance_type="g4dn.xlarge",
            region_name="us-east-1",
            max_size=1,
            min_size=0,
            username=module_user,
        ),
    )

    task = queryStatus(client, resp, timeout=10)

    if task["status"] < 1:
        fractal_logger.error(task["output"])
        assert False
    if not ClusterInfo.query.get(cluster_name):
        fractal_logger.error("Cluster was not inserted in database")
        assert False
    assert True


@pytest.mark.container_serial
@pytest.mark.usefixtures("celery_worker")
def test_assign_container(client, module_user, monkeypatch, task_def_env):
    client.login(module_user, admin=True)
    monkeypatch.setattr(aws_ecs_creation, "_poll", function(returns=True))
    fractal_logger.info("Starting to assign container in cluster {}".format(pytest.cluster_name))

    resp = client.post(
        "/aws_container/assign_mandelbox",
        json=dict(
            username=module_user,
            cluster_name=pytest.cluster_name,
            region_name="us-east-1",
            task_definition_arn="fractal-{}-browsers-chrome".format(task_def_env),
        ),
    )

    task = queryStatus(client, resp, timeout=50)

    if task["status"] < 1:
        fractal_logger.error(task["output"])
        assert False

    if not task["result"]:
        fractal_logger.error("No container returned")
        assert False
    pytest.container_name = task["result"]["container_id"]

    container_name = UserContainer.query.get(pytest.container_name)
    if container_name is None:
        fractal_logger.error("Container was not inserted in database")
        assert False

    assert True
    return task["result"]


@pytest.mark.usefixtures("celery_worker")
def test_update_cluster(monkeypatch, task_def_env):
    success = False

    def mock_prewarm(
        task_definition_arn, task_version, cluster_name, region_name, webserver_url
    ):  # pylint:disable=unused-argument
        # ensure:
        # - task def is chrome for the correct task environment
        # - cluster is correct
        nonlocal success
        success = (task_definition_arn == f"fractal-{task_def_env}-browsers-chrome") and (
            cluster_name == pytest.cluster_name
        )

    # mock calling prewarm new container because we are upgrading the AMI to something that will
    # not support prewarming a container
    monkeypatch.setattr(prewarm_new_container, "run", mock_prewarm)

    # right now we have manually verified this actually does something on AWS.
    # AWS/boto3 _should_ error out if something went wrong.
    res = update_cluster.delay(
        cluster_name=pytest.cluster_name,
        webserver_url=None,  # will go to dev
        region_name="us-east-1",
        ami=GENERIC_UBUNTU_SERVER_2004_LTS_AMI,
    )

    # wait for operation to finish
    res.get(timeout=30)

    # make sure mock prewarm was called and succeeded
    assert success is True
    # make sure task succeeded
    assert res.successful()
    assert res.state == "SUCCESS"


@pytest.mark.usefixtures("celery_worker")
def test_update_bad_cluster(cluster):
    # Regression test for PR 665, tests that a dead cluster doesn't brick
    res = update_cluster.delay(
        cluster_name=cluster.cluster,
        webserver_url=None,
        region_name="us-east-1",
        ami=GENERIC_UBUNTU_SERVER_2004_LTS_AMI,
    )

    # wait for operation to finish
    # if it takes more than 30 sec, something is wrong
    res.get(timeout=30)

    assert res.successful()
    assert res.state == "SUCCESS"


@pytest.mark.usefixtures("celery_worker")
def test_delete_bad_cluster():
    """Test that `delete_cluster` handles invalid clusters.

    This tests how `delete_cluster` behaves when passed a dummy cluster,
    simulating the behavior that occurs when we manually delete an AWS cluster
    without removing its entry from the database, and `delete_cluster` later
    gets called on it.
    """

    # This cluster doesn't actually exist on our AWS account.
    cluster_name = ECSClient.generate_name("cluster", test_prefix=True)
    cluster = ClusterInfo(cluster=cluster_name, location="us-east-1")

    db.session.add(cluster)  # Change the cluster object's state to pending.
    db.session.flush()  # Change the cluster object's state to persistent.

    # The cluster object's state must be persistent so it can be merged into the worker process's
    # transaction with load=False at the beginning of delete_cluster.run()
    task = delete_cluster.delay(cluster, force=False)

    task.get()

    assert task.successful()


def test_ensure_container_exists(container, monkeypatch):
    """Test that `ensure_container_exists` handles bad inputs correctly.

    This first tests that passing in an argument of `None` raises the
    expected exception. Next, it tests calling this on a dummy container
    that claims it is in the default us-east-1 cluster, and ensures that
    the dummy container is correctly deleted from the database.
    """

    # Make sure it handles None input appropriately.
    # The function should throw an exception here.
    with pytest.raises(Exception, match=r"Invalid parameter: `container` is `None`"):
        ensure_container_exists(None)

    # We could test here that this works on a real container that
    # does in fact belong to a cluster. However, we do already
    # effectively test this in our tests for `assign_container`
    # and `delete_container`; therefore, we omit this explicit
    # test for the sake of simplicity.

    # Make sure it handles fake containers properly.
    with container() as dummy_container:
        # Instead of using the dummy cluster, use the default cluster for
        # us-east-1 as the target for our ECS query. It is guaranteed that
        # this fake container doesn't exist in this real cluster.
        # To be clear, I am hardcoding this string based on the expectation
        # that the default cluster in us-east-1 will always be named "default".
        monkeypatch.setattr(dummy_container, "cluster", "default")

        response_container = ensure_container_exists(dummy_container)

        # First of all, on a dummy container that does not exist, this function
        # should return None.
        if response_container:
            fractal_logger.error("Bad container input did not return None.")
            assert False

        # Second, on a dummy container that does not exist, this function
        # should have deleted the erroneous entry from the database.
        query_result = UserContainer.query.get(dummy_container.container_id)
        if query_result:
            fractal_logger.error("Bad container input did not delete erroneous entry from db.")
            assert False

        # Nothing went wrong!
        assert True


@pytest.mark.container_serial
@pytest.mark.usefixtures("celery_worker")
def test_delete_container(client, monkeypatch):
    fractal_logger.info("Starting to delete container {}".format(pytest.container_name))

    container = UserContainer.query.get(pytest.container_name)
    if container is None:
        fractal_logger.info("No containers returned by UserContainer db")
        assert False

    # delete_container will call manual_scale_cluster
    # it should follow through the logic until it actually tries to kill the instance,
    # which we don't need to do since delete_cluster handles it.
    # this in essence tests the logic right up to set_auto_scaling_group_capacity
    def mock_set_capacity(self, asg_name: str, desired_capacity: int):
        setattr(mock_set_capacity, "test_passed", desired_capacity == 0)

    monkeypatch.setattr(ECSClient, "set_auto_scaling_group_capacity", mock_set_capacity)

    resp = client.post(
        "/mandelbox/delete",
        json=dict(
            private_key=container.secret_key,
            container_id=pytest.container_name,
        ),
    )

    task = queryStatus(client, resp, timeout=10)

    if task["status"] < 1:
        fractal_logger.error(task["output"])
        assert False

    db.session.expire(container)

    if UserContainer.query.get(pytest.container_name):
        fractal_logger.error("Container was not deleted from database")
        assert False

    assert hasattr(mock_set_capacity, "test_passed")  # make sure function was called
    assert getattr(mock_set_capacity, "test_passed")  # make sure function passed


@pytest.mark.usefixtures("authorized")
@pytest.mark.usefixtures("celery_worker")
def test_update_region(client, monkeypatch):
    # this makes update_cluster behave like dummy_update_cluster. undone after test finishes.
    # we use update_cluster.delay in update_region, but here we override with a mock
    def mock_update_cluster(
        cluster_name=None, webserver_url=None, region_name="us-east-1", ami=None
    ):
        success = True
        # check that the arguments are as expected
        if cluster_name != pytest.cluster_name:
            fractal_logger.error(f"Expected cluster {pytest.cluster_name}, got {cluster_name}")
            success = False
        elif region_name != "us-east-1":
            fractal_logger.error(f"Expected region us-east-1, got {region_name}")
            success = False
        elif ami != GENERIC_UBUNTU_SERVER_2004_LTS_AMI:
            fractal_logger.error(f"Expected ami {GENERIC_UBUNTU_SERVER_2004_LTS_AMI}, got {ami}")
            success = False

        # tests looks for this attribute on the function
        if hasattr(mock_update_cluster, "test_passed"):
            # this means this func has been called twice, which should not happen
            setattr(mock_update_cluster, "test_passed", False)
            raise ValueError(
                f"mock_update_cluster called twice, second time with cluster {cluster_name}"
            )
        setattr(mock_update_cluster, "test_passed", success)

        class FakeReturn:
            def __init__(self):
                self.id = "this-is-a-fake-celery-test-id"

        return FakeReturn()

    # do monkeypatching
    monkeypatch.setattr(update_cluster, "delay", mock_update_cluster)

    fractal_logger.info("Calling update_region with monkeypatched update_cluster")

    db.session.expire_all()
    all_regions_pre = RegionToAmi.query.all()
    region_to_ami_pre = {region.region_name: region.ami_id for region in all_regions_pre}

    # -- actual webserver requests start -- #
    # this should fail cause server is not in maintenance mode
    resp = client.post(
        "/aws_container/update_region",
        json=dict(
            region_name="us-east-1",
            ami=GENERIC_UBUNTU_SERVER_2004_LTS_AMI,
        ),
    )
    assert resp.status_code == BAD_REQUEST

    # then, we put server into maintenance mode
    resp = client.post("/aws_container/start_maintenance")
    assert resp.status_code == SUCCESS
    assert resp.json["success"] is True

    # now we try again
    resp = client.post(
        "/aws_container/update_region",
        json=dict(
            region_name="us-east-1",
            ami=GENERIC_UBUNTU_SERVER_2004_LTS_AMI,
        ),
    )

    task = queryStatus(client, resp, timeout=30)
    if task["status"] < 1:
        fractal_logger.error(task["output"])
        assert False

    # finally, we end maintenance mode
    resp = client.post("/aws_container/end_maintenance")
    assert resp.status_code == SUCCESS
    assert resp.json["success"] is True
    # -- webserver requests end -- #

    db.session.expire_all()
    all_regions_post = RegionToAmi.query.all()
    region_to_ami_post = {region.region_name: region.ami_id for region in all_regions_post}
    for region in region_to_ami_post:
        if region == "us-east-1":
            assert region_to_ami_post[region] == GENERIC_UBUNTU_SERVER_2004_LTS_AMI
            # restore db to old (correct) AMI in case any future tests need it
            region_to_ami = RegionToAmi.query.filter_by(
                region_name="us-east-1",
            ).first()
            region_to_ami.ami_id = region_to_ami_pre["us-east-1"]
            fractal_sql_commit(db)
        else:
            # nothing else in db should change
            assert region_to_ami_post[region] == region_to_ami_pre[region]

    assert hasattr(mock_update_cluster, "test_passed"), "mock_update_cluster was never called!"
    assert getattr(mock_update_cluster, "test_passed")


@pytest.mark.container_serial
@pytest.mark.usefixtures("authorized")
@pytest.mark.usefixtures("celery_worker")
def test_delete_cluster(client):
    cluster = pytest.cluster_name
    fractal_logger.info("Starting to delete cluster {}".format(cluster))

    resp = client.post(
        "/aws_container/delete_cluster",
        json=dict(
            cluster_name=pytest.cluster_name,
            region_name="us-east-1",
        ),
    )

    task = queryStatus(client, resp, timeout=10)

    if task["status"] < 1:
        fractal_logger.error(task["output"])
        assert False
    if ClusterInfo.query.get(cluster):
        fractal_logger.error("Cluster was not deleted in database")
        assert False
    assert True


# this test does not need AWS resources or any cluster/container setup
@pytest.mark.usefixtures("authorized")
@pytest.mark.usefixtures("celery_worker")
def test_update_single_taskdef(client, monkeypatch):
    # mock describe_task so we don't unnecesarily call AWS API
    # this also stops races between someone running tests and task definition updates that
    # are incompatible with the AMIs that the test suite started with.
    fake_aws_response = {"taskDefinition": {"revision": -1}}
    monkeypatch.setattr(ECSClient, "describe_task", function(returns=fake_aws_response))

    app_id = "Firefox"
    # cache the current task defs
    db.session.expire_all()
    all_app_data = SupportedAppImages.query.all()
    app_id_to_version = {app_data.app_id: app_data.task_version for app_data in all_app_data}

    # -- actual webserver requests start -- #
    # this should fail cause server is not in maintenance mode
    resp = client.post("/aws_container/update_taskdefs")
    assert resp.status_code == BAD_REQUEST

    # then, we put server into maintenance mode
    resp = client.post("/aws_container/start_maintenance")
    assert resp.status_code == SUCCESS
    assert resp.json["success"] is True

    # now we try again
    resp = client.post("/aws_container/update_taskdefs", json={"app_id": app_id})
    task = queryStatus(client, resp, timeout=2)
    if task["status"] < 1:
        fractal_logger.error(f"task timed out with output {task['output']}")
        assert False

    # finally, we end maintenance mode
    resp = client.post("/aws_container/end_maintenance")
    assert resp.status_code == SUCCESS
    assert resp.json["success"] is True
    # -- webserver requests end -- #

    # refresh db objects
    db.session.expire_all()
    all_app_data = SupportedAppImages.query.all()
    for app_data in all_app_data:
        if app_data.app_id == app_id:
            assert app_data.task_version == -1
            # the task version should be different than the cached
            assert app_data.task_version != app_id_to_version[app_data.app_id]
        else:
            # everything else should be the same as cached
            assert app_data.task_version == app_id_to_version[app_data.app_id]

    # restore db to old state
    db.session.expire_all()
    app_data = SupportedAppImages.query.get(app_id)
    app_data.task_version = app_id_to_version[app_data.app_id]
    db.session.commit()


# this test does not need AWS resources or any cluster/container setup
@pytest.mark.usefixtures("authorized")
@pytest.mark.usefixtures("celery_worker")
def test_update_all_taskdefs(client, monkeypatch):
    # mock describe_task so we don't unnecesarily call AWS API
    # this also stops races between someone running tests and task definition updates that
    # are incompatible with the AMIs that the test suite started with.
    fake_aws_response = {"taskDefinition": {"revision": -1}}
    monkeypatch.setattr(ECSClient, "describe_task", function(returns=fake_aws_response))

    # cache the current task defs
    db.session.expire_all()
    all_app_data = SupportedAppImages.query.all()
    app_id_to_version = {app_data.app_id: app_data.task_version for app_data in all_app_data}

    # -- actual webserver requests start -- #
    # this should fail cause server is not in maintenance mode
    resp = client.post("/aws_container/update_taskdefs")
    assert resp.status_code == BAD_REQUEST

    # then, we put server into maintenance mode
    resp = client.post("/aws_container/start_maintenance")
    assert resp.status_code == SUCCESS
    assert resp.json["success"] is True

    # now we try again
    resp = client.post("/aws_container/update_taskdefs")
    task = queryStatus(client, resp, timeout=2)
    if task["status"] < 1:
        fractal_logger.error(f"task timed out with output {task['output']}")
        assert False

    # finally, we end maintenance mode
    resp = client.post("/aws_container/end_maintenance")
    assert resp.status_code == SUCCESS
    assert resp.json["success"] is True
    # -- webserver requests end -- #

    # refresh db objects
    db.session.expire_all()
    all_app_data = SupportedAppImages.query.all()
    for app_data in all_app_data:
        assert app_data.task_version == -1
        # the task version should be different than the cached
        assert app_data.task_version != app_id_to_version[app_data.app_id]

    # restore db to old state
    db.session.expire_all()
    all_app_data = SupportedAppImages.query.all()
    for app_data in all_app_data:
        app_data.task_version = app_id_to_version[app_data.app_id]
    db.session.commit()


def test_update_taskdef_fk_regression(bulk_container):
    """
    A regression test to make sure task definitions versions can be updated
    independent of user containers that use an old taskdef version. This used
    to be a foreign key issue that errored out.
    """

    app_data = SupportedAppImages.query.get("Firefox")
    _ = bulk_container(assigned_to=None)

    old_version = app_data.task_version
    assert old_version != -1  # sanity check. nothing in the db should have this value
    app_data.task_version = -1

    # this should cause no problems. We now have a db where UserContainers has a
    # user running an old taskdef and SupportedAppImages has upgraded to a new version
    db.session.commit()

    # undo changes
    app_data.task_version = old_version
    db.session.commit()


@pytest.mark.usefixtures("celery_worker")
def test_manual_scale_cluster_up(bulk_cluster, monkeypatch):
    """
    Unit test for scaling a cluster up. Mocks all AWS functions.
    """
    cluster = bulk_cluster(
        location="fake-region",
        # this should be a scale-up
        pendingTasksCount=6,
        runningTasksCount=5,
        registeredContainerInstancesCount=1,
        minContainers=1,
        maxContainers=10,
    )

    def mock_set_asg_capacity(
        self, asg_name: str, desired_capacity: int
    ):  # pylint: disable=unused-argument
        # manual_scale_cluster should try to increase capacity to 2
        setattr(mock_set_asg_capacity, "test_passed", desired_capacity == 2)

    # mock kill_draining because the cluster does not actually exist in ECS
    monkeypatch.setattr(
        aws_ecs_modification, "kill_draining_instances_with_no_tasks", function(returns=0)
    )
    monkeypatch.setattr(
        ECSClient, "get_auto_scaling_groups_in_cluster", function(returns=["fake-asg"])
    )
    monkeypatch.setattr(ECSClient, "set_auto_scaling_group_capacity", mock_set_asg_capacity)

    manual_scale_cluster.delay(cluster.cluster, cluster.location).get()

    # make sure the mock function was called with the right desired capacity
    assert hasattr(mock_set_asg_capacity, "test_passed")
    assert getattr(mock_set_asg_capacity, "test_passed")


@pytest.mark.usefixtures("celery_worker")
def test_manual_scale_cluster_down(bulk_cluster, monkeypatch):
    """
    Unit test for scaling a cluster down. Mocks all AWS functions. By "simple",
    we mean that there is an obvious instance to delete.
    """
    cluster = bulk_cluster(
        location="fake-region",
        # this should be a scale-down
        pendingTasksCount=5,
        runningTasksCount=4,
        registeredContainerInstancesCount=2,
        minContainers=1,
        maxContainers=10,
    )

    def mock_set_asg_capacity(
        self, asg_name: str, desired_capacity: int
    ):  # pylint: disable=unused-argument
        # manual_scale_cluster should try to increase capacity to 2
        setattr(mock_set_asg_capacity, "test_passed", desired_capacity == 1)

    # mock kill_draining because the cluster does not actually exist in ECS
    monkeypatch.setattr(
        aws_ecs_modification, "kill_draining_instances_with_no_tasks", function(returns=0)
    )
    monkeypatch.setattr(
        ECSClient, "get_auto_scaling_groups_in_cluster", function(returns=["fake-asg1"])
    )
    monkeypatch.setattr(ECSClient, "set_auto_scaling_group_capacity", mock_set_asg_capacity)

    # now we call the function
    manual_scale_cluster.delay(cluster.cluster, cluster.location).get()

    # make sure the mock function was called with the right desired capacity
    assert hasattr(mock_set_asg_capacity, "test_passed")
    assert getattr(mock_set_asg_capacity, "test_passed")
