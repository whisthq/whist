import copy
import logging
import os
import uuid

from collections import defaultdict

import pytest
import celery
from celery import shared_task

from app.celery.aws_ecs_modification import update_cluster
from app.celery.aws_ecs_creation import _poll
from app.helpers.utils.general.logs import fractal_log
from app.helpers.utils.general.sql_commands import fractal_sql_commit
from app.models import ClusterInfo, db, UserContainer, RegionToAmi

from ..helpers.general.progress import fractalJobRunner, queryStatus

from app.helpers.utils.aws.base_ecs_client import ECSClient
from app.celery.aws_ecs_modification import manual_scale_cluster


pytest.cluster_name = f"test-cluster-{uuid.uuid4()}"
pytest.container_name = None


@pytest.mark.container_serial
def check_test_database():
    if os.getenv("HEROKU_APP_NAME") == "main-webserver":
        fractal_log(
            function="test_aws_container",
            label=None,
            logs="Not using staging database or resource group! Forcefully stopping tests.",
            level=logging.WARNING,
        )
        assert False


@pytest.mark.container_serial
@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
@pytest.mark.usefixtures("_save_user")
def test_create_cluster(client, admin, cluster_name=pytest.cluster_name):
    cluster_name = cluster_name or pytest.cluster_name
    fractal_log(
        function="test_create_cluster",
        label="cluster/create",
        logs="Starting to create cluster {}".format(cluster_name),
    )

    resp = client.post(
        "/aws_container/create_cluster",
        json=dict(
            cluster_name=cluster_name,
            instance_type="g3s.xlarge",
            region_name="us-east-1",
            max_size=1,
            min_size=0,
            username=admin.user_id,
        ),
    )

    task = queryStatus(client, resp, timeout=10)

    if task["status"] < 1:
        fractal_log(
            function="test_create_cluster",
            label="cluster/create",
            logs=task["output"],
            level=logging.ERROR,
        )
        assert False
    if not ClusterInfo.query.get(cluster_name):
        fractal_log(
            function="test_create_cluster",
            label="cluster/create",
            logs="Cluster was not inserted in database",
            level=logging.ERROR,
        )
        assert False
    assert True


@pytest.mark.container_serial
@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
@pytest.mark.usefixtures("_retrieve_user")
@pytest.mark.usefixtures("_save_user")
def test_assign_container(client, admin, monkeypatch):
    monkeypatch.setattr(_poll, "__code__", (lambda *args, **kwargs: True).__code__)

    deploy_env = "dev"
    if os.getenv("HEROKU_APP_NAME") == "fractal-prod-server":
        deploy_env = "prod"
    elif os.getenv("HEROKU_APP_NAME") == "fractal-staging-server":
        deploy_env = "staging"

    fractal_log(
        function="test_assign_container",
        label="container/assign",
        logs="Starting to assign container in cluster {}".format(pytest.cluster_name),
    )
    resp = client.post(
        "/aws_container/assign_container",
        json=dict(
            username=admin.user_id,
            cluster_name=pytest.cluster_name,
            region_name="us-east-1",
            task_definition_arn="fractal-{}-browsers-chrome".format(deploy_env),
        ),
    )

    task = queryStatus(client, resp, timeout=50)

    if task["status"] < 1:
        fractal_log(
            function="test_assign_container",
            label="container/assign",
            logs=task["output"],
            level=logging.ERROR,
        )
        assert False

    if not task["result"]:
        fractal_log(
            function="test_assign_container",
            label="container/assign",
            logs="No container returned",
            level=logging.ERROR,
        )
        assert False
    pytest.container_name = task["result"]["container_id"]

    container_name = UserContainer.query.get(pytest.container_name)
    if container_name is None:
        fractal_log(
            function="test_assign_container",
            label="container/assign",
            logs="Container was not inserted in database",
            level=logging.ERROR,
        )
        assert False

    assert True
    return task["result"]


@pytest.mark.container_serial
@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
@pytest.mark.usefixtures("_retrieve_user")
@pytest.mark.usefixtures("_save_user")
@pytest.mark.usefixtures("admin")
def test_send_commands(client):
    fractal_log(
        function="test_send_commands",
        label="cluster/send_commands",
        logs="Starting to send commands to cluster {}".format(pytest.cluster_name),
    )

    resp = client.post(
        "/aws_container/send_commands",
        json=dict(
            cluster=pytest.cluster_name,
            region_name="us-east-1",
            commands=["echo test_send_commands"],
        ),
    )

    task = queryStatus(client, resp, timeout=10)

    if task["status"] < 1:
        fractal_log(
            function="test_send_commands",
            label="cluster/send_commands",
            logs=task["output"],
            level=logging.ERROR,
        )
        assert False

    assert True


@pytest.mark.container_serial
@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
def test_update_cluster(client):
    # right now we have manually verified this actually does something on AWS.
    # AWS/boto3 _should_ error out if something went wrong.
    res = update_cluster.delay(
        region_name="us-east-1",
        cluster_name=pytest.cluster_name,
        ami="ami-0ff8a91507f77f867",  # a generic Linux AMI
    )

    # wait for operation to finish
    res.get(timeout=30)

    assert res.successful()
    assert res.state == "SUCCESS"


@pytest.mark.container_serial
@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
def test_update_bad_cluster(client, cluster):
    # right now we have manually verified this actually does something on AWS.
    # AWS/boto3 _should_ error out if something went wrong.
    res = update_cluster.delay(
        region_name="us-east-1",
        cluster_name=cluster.cluster,
        ami="ami-0ff8a91507f77f867",  # a generic Linux AMI
    )

    # wait for operation to finish
    res.get(timeout=30)

    assert res.successful()
    assert res.state == "SUCCESS"


@pytest.mark.container_serial
@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
def test_delete_container(client, monkeypatch):
    fractal_log(
        function="test_delete_container",
        label="container/delete",
        logs="Starting to delete container {}".format(pytest.container_name),
    )

    container = UserContainer.query.get(pytest.container_name)
    if container is None:
        fractal_log(
            "test_delete_container",
            label="",
            logs="No containers returned by UserContainer db",
        )
        assert False

    # delete_container will call manual_scale_cluster
    # it should follow through the logic until it actually tries to kill the instance,
    # which we don't need to do since delete_cluster handles it. hence we mock it.
    def mock_set_capacity(self, asg_name: str, desired_capacity: int):
        setattr(mock_set_capacity, "test_passed", desired_capacity == 0)

    monkeypatch.setattr(ECSClient, "set_auto_scaling_group_capacity", mock_set_capacity)

    resp = client.post(
        "/container/delete",
        json=dict(
            private_key=container.secret_key,
            container_id=pytest.container_name,
        ),
    )

    task = queryStatus(client, resp, timeout=10)

    if task["status"] < 1:
        fractal_log(
            function="test_delete_container",
            label="container/delete",
            logs=task["output"],
            level=logging.ERROR,
        )
        assert False

    db.session.expire(container)

    if UserContainer.query.get(pytest.container_name):
        fractal_log(
            function="test_delete_container",
            label="container/delete",
            logs="Container was not deleted from database",
            level=logging.ERROR,
        )
        assert False

    assert hasattr(mock_set_capacity, "test_passed")  # make sure function was called
    assert getattr(mock_set_capacity, "test_passed")  # make sure function passed


@pytest.mark.container_serial
@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
@pytest.mark.usefixtures("_save_user")
def test_update_region(client, admin, monkeypatch):
    # this makes update_cluster behave like dummy_update_cluster. undone after test finishes.
    # we use update_cluster.delay in update_region, but here we override with a mock
    def mock_update_cluster(region_name="us-east-1", cluster_name=None, ami=None):
        success = True
        # check that the arguments are as expected
        if cluster_name != pytest.cluster_name:
            fractal_log(
                "mock_update_cluster",
                None,
                f"Expected cluster {pytest.cluster_name}, got {cluster_name}",
                logging.ERROR,
            )
            success = False
        elif region_name != "us-east-1":
            fractal_log(
                "mock_update_cluster",
                None,
                f"Expected region us-east-1, got {region_name}",
                logging.ERROR,
            )
            success = False
        elif ami != "ami-0ff8a91507f77f867":  # a generic Linux AMI
            fractal_log(
                "mock_update_cluster",
                None,
                f"Expected ami ami-0ff8a91507f77f867, got {ami}",
                logging.ERROR,
            )
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
                self.id = "this-is-a-fake-test-id"

        return FakeReturn()

    # do monkeypatching
    monkeypatch.setattr(update_cluster, "delay", mock_update_cluster)

    fractal_log(
        function="test_update_region",
        label=None,
        logs="Calling update_region with monkeypatched update_cluster",
    )

    db.session.expire_all()
    all_regions_pre = RegionToAmi.query.all()
    region_to_ami_pre = {region.region_name: region.ami_id for region in all_regions_pre}

    resp = client.post(
        "/aws_container/update_region",
        json=dict(
            region_name="us-east-1",
            ami="ami-0ff8a91507f77f867",  # a generic Linux AMI
        ),
    )

    task = queryStatus(client, resp, timeout=30)
    if task["status"] < 1:
        fractal_log(
            function="test_update_region",
            label=None,
            logs=task["output"],
            level=logging.ERROR,
        )
        assert False

    db.session.expire_all()
    all_regions_post = RegionToAmi.query.all()
    region_to_ami_post = {region.region_name: region.ami_id for region in all_regions_post}
    for region in region_to_ami_post:
        if region == "us-east-1":
            assert region_to_ami_post[region] == "ami-0ff8a91507f77f867"  # a generic Linux AMI
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
@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
@pytest.mark.usefixtures("_retrieve_user")
@pytest.mark.usefixtures("admin")
def test_delete_cluster(client, cluster=pytest.cluster_name):
    cluster = cluster or pytest.cluster_name
    fractal_log(
        function="test_delete_cluster",
        label="cluster/delete",
        logs="Starting to delete cluster {}".format(cluster),
    )

    resp = client.post(
        "/aws_container/delete_cluster",
        json=dict(
            cluster_name=pytest.cluster_name,
            region_name="us-east-1",
        ),
    )

    task = queryStatus(client, resp, timeout=10)

    if task["status"] < 1:
        fractal_log(
            function="test_delete_cluster",
            label="cluster/delete",
            logs=task["output"],
            level=logging.ERROR,
        )
        assert False
    if ClusterInfo.query.get(cluster):
        fractal_log(
            function="test_delete_cluster",
            label="cluster/delete",
            logs="Cluster was not deleted in database",
            level=logging.ERROR,
        )
        assert False
    assert True


@pytest.mark.skipif(
    "CHOOSE_CLUSTER_TEST" not in os.environ,
    reason="This test is slow and reource heavy; run only upon explicit request",
)
def test_cluster_assignment():
    check_test_database(input_token, admin_token)
    fractal_log(
        function="test_cluster_assignment",
        label="cluster/assign",
        logs="First deleting all clusters in database",
    )

    def delete_cluster_helper(cluster):
        fractal_log(
            function="test_cluster_assignment",
            label="cluster/delete",
            logs="Deleting cluster {} from database".format(cluster),
        )
        cluster_info = ClusterInfo.query.get(cluster)
        fractal_sql_commit(db, lambda db, x: db.session.delete(x), cluster_info)

        if ClusterInfo.query.get(cluster):
            fractal_log(
                function="test_cluster_assignment",
                label="cluster/delete",
                logs="Cluster was not deleted in database",
                level=logging.ERROR,
            )
            assert False
        assert True

    all_clusters = ClusterInfo.query.all()
    clusters = [
        cluster_info.cluster for cluster_info in all_clusters if "cluster_" in cluster_info.cluster
    ]
    if all_clusters:
        fractal_log(
            function="test_cluster_assignment",
            label="cluster/delete",
            logs="Found {num_clusters} cluster(s) already in database. Starting to delete...".format(
                num_clusters=str(len(clusters))
            ),
        )
        fractalJobRunner(delete_cluster_helper, clusters)

    else:
        fractal_log(
            function="test_cluster_assignment",
            label="cluster/delete",
            logs="No clusters found in database",
        )

    first_cluster = f"test-cluster-{uuid.uuid4()}"
    test_create_cluster(input_token, admin_token, max_containers=2, cluster_name=first_cluster)
    clusters_to_containers = defaultdict(list)
    container_cluster = first_cluster
    container_cluster_info = ClusterInfo.query.filter_by(cluster=container_cluster).first()

    while True:
        pytest.cluster_name = None
        container_info = test_create_container(input_token, admin_token, cluster_name=None)
        new_container, new_container_cluster = (
            container_info["container_id"],
            container_info["cluster"],
        )
        clusters_to_containers[new_container_cluster].append(new_container)
        old_info = copy.copy(container_cluster_info)
        db.session.expire(container_cluster_info)
        container_cluster_info = ClusterInfo.query.get(new_container_cluster)

        if (
            old_info.registeredContainerInstancesCount < old_info.maxContainers
            or old_info.maxMemoryRemainingPerInstance > 8500
        ):
            assert container_cluster == new_container_cluster
            assert (
                container_cluster_info.maxMemoryRemainingPerInstance
                <= old_info.maxMemoryRemainingPerInstance
                or container_cluster_info.registeredContainerInstancesCount
                > old_info.registeredContainerInstancesCount
            )
        else:
            assert container_cluster != new_container_cluster
            assert (
                container_cluster_info.maxMemoryRemainingPerInstance
                >= old_info.maxMemoryRemainingPerInstance
            )
            db.session.expire(container_cluster_info)
            break

        container_cluster = new_container_cluster
    first_cluster_info = ClusterInfo.query.get(first_cluster)
    old_first_cluster_info = copy.copy(first_cluster_info)
    container_to_delete = clusters_to_containers[first_cluster].pop()
    test_delete_container(input_token, admin_token, container_to_delete)
    db.session.refresh(first_cluster_info)
    new_first_cluster_info = copy.copy(first_cluster_info)
    assert (
        new_first_cluster_info.maxMemoryRemainingPerInstance
        >= old_first_cluster_info.maxMemoryRemainingPerInstance
    )

    pytest.cluster_name = None
    container_info = test_create_container(input_token, admin_token, cluster_name=None)
    new_container, assigned_cluster = container_info["container_id"], container_info["cluster"]
    clusters_to_containers[assigned_cluster].append(new_container)
    assert assigned_cluster == first_cluster
    db.session.refresh(first_cluster_info)
    assigned_cluster_info = copy.copy(first_cluster_info)
    assert (
        assigned_cluster_info.maxMemoryRemainingPerInstance
        <= new_first_cluster_info.maxMemoryRemainingPerInstance
    )
    db.session.expire(first_cluster_info)

    for cluster in clusters_to_containers.keys():
        for container in clusters_to_containers[cluster]:
            test_delete_container(input_token, admin_token, container)
        test_delete_cluster(input_token, admin_token, cluster)
