"""Unit and integration tests for code that we use to manipulate ECS clusters."""

import uuid

import boto3
import pytest

from sqlalchemy.orm.exc import NoResultFound

from app.celery.aws_ecs_creation import create_new_cluster
from app.celery.aws_ecs_deletion import delete_cluster
from app.helpers.utils.aws import autoscaling, ecs
from app.helpers.utils.aws.base_ecs_client import ECSClient
from app.models import ClusterInfo, db

from ..patches import function


@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
@pytest.mark.parametrize("region", ("us-east-1",))
def test_lifecycle(region):
    """Test creation and deletion of an ECS cluster through Fractal's web API."""

    cluster_name = f"test-cluster-{uuid.uuid4()}"
    ecs_client = boto3.client("ecs", region_name=region)

    create_new_cluster.delay(
        cluster_name=cluster_name, region_name=region, max_size=0, ami=None
    ).get()

    # Make sure the cluster actually got created.
    cluster = ClusterInfo.query.filter_by(cluster=cluster_name, location=region).one()

    assert ecs_client.describe_clusters(clusters=(cluster_name,))[
        "clusters"
    ], f"Failed to create {cluster_name} in {region}"

    delete_cluster.delay(cluster, force=False).get()
    db.session.expunge(cluster)

    # Make sure the cluster was actually deleted. The list of clusters returned by this request
    # should either be empty or contain a single cluster. If the list is empty, the cluster has
    # been deleted. If the list is non-empty, the only cluster in the list should be in one of the
    # "DEPROVISIONING" or "INACTIVE" states.
    clusters = ecs_client.describe_clusters(clusters=(cluster_name,))["clusters"]

    assert len(clusters) in (0, 1)  # Sanity check

    if len(clusters) == 1:
        assert clusters[0]["clusterName"] == cluster_name
        assert clusters[0]["status"] in ("DEPROVISIONING", "INACTIVE")

    with pytest.raises(NoResultFound):
        ClusterInfo.query.filter_by(cluster=cluster_name, location=region).one()


@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
def test_delete_failure(cluster, monkeypatch):
    """Make sure the cluster pointer is added back to the database if any AWS queries fail.

    We can be sure that the cluster pointer is reinstated if the teardown code for the cluster
    fixture raises no errors because the cluster fixture teardown code will raise an error if the
    cluster pointer that it wants to delete is missing from the database.
    """

    monkeypatch.setattr(ECSClient, "delete_cluster", function(raises=Exception))

    with pytest.raises(Exception):
        delete_cluster.delay(cluster, force=False).get()


def test_delete_bad_autoscaling_group():
    """Handle deletion of missing auto-scaling groups gracefully.

    The delete_autoscaling_group() function should return None if the auto-scaling group to be
    deleted doesn't exist.
    """

    assert autoscaling.delete_autoscaling_group("not-an-autoscaling-group", "us-east-1") is None


def test_delete_bad_launch_configuration():
    """Handle deletion of missing launch configurations gracefully."""

    assert (
        autoscaling.delete_launch_configuration("not-a-launch-configuration", "us-east-1") is None
    )


def test_get_launch_configuration_from_bad_autoscaling_group():
    """Don't freak out if the auto-scaling group doesn't exist."""

    assert autoscaling.get_launch_configuration("not-an-autoscaling-group", "us-east-1") is None


def test_delete_bad_capacity_provider():
    """Handle deletion of missing capacity providers gracefully.

    The delete_capacity_provider() function should return None if the capacity provider to be
    deleted doesn't exist.
    """

    assert ecs.delete_capacity_provider("not-a-capacity-provider", "us-east-1") is None


def test_delete_bad_cluster():
    """Handle deletion of missing clusters gracefully.

    The delete_cluster() function should return an empty list (of capacity provider names) if the
    cluster to be deleted doesn't exist.
    """

    assert ecs.delete_cluster("not-a-cluster", "us-east-1") == []


def test_deregister_container_instances_from_bad_cluster():
    """Don't freak out if the cluster doesn't exist."""

    assert ecs.deregister_container_instances("not-a-cluster", "us-east-1") is None
