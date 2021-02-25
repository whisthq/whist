"""Unit and integration tests for code that we use to manipulate ECS clusters."""

import uuid

from collections import namedtuple

import boto3
import pytest

from sqlalchemy.orm.exc import NoResultFound

from app.celery.aws_ecs_creation import create_new_cluster
from app.celery.aws_ecs_deletion import delete_cluster
from app.helpers.utils.aws import autoscaling, ecs
from app.helpers.utils.aws.base_ecs_client import ECSClient
from app.models import ClusterInfo, db

from ..patches import function

_Stack = namedtuple(
    "_Stack",
    ("cluster", "capacity_provider", "autoscaling_group", "launch_configuration", "region"),
)


class Stack(_Stack):
    """Represents an ECS stack.

    Attributes:
        autoscaling_group: The short name of the capacity provider's auto-scaling group as a
            string.
        capacity_provider: The short name of the cluster's capacity provider as a string.
        cluster: The short name of the ECS cluster as a string.
        launch_configuration: The short name of the auto-scaling group's launch configuration as a
            string.
    """

    @classmethod
    def of(cls, cluster, region):
        """Query ECS for the names of the remaining members of a particular cluster's ECS stack.

        This is not a particularly well-error-handled constructor method.

        Args:
            cluster: The short name of the cluster whose stack is to be represented as a string.
            region: The region in which the cluster is located as a string (e.g. "us-east-1").

        Returns:
            An instance of the Stack class representing the specified cluster's ECS stack.
        """

        ecs_client = boto3.client("ecs", region_name=region)
        cluster_data = ecs_client.describe_clusters(clusters=(cluster,))
        capacity_provider = cluster_data["clusters"][0]["capacityProviders"][0]
        capacity_provider_data = ecs_client.describe_capacity_providers(
            capacityProviders=(capacity_provider,)
        )
        _, autoscaling_group = capacity_provider_data["capacityProviders"][0][
            "autoScalingGroupProvider"
        ]["autoScalingGroupArn"].rsplit("/", maxsplit=1)
        launch_configuration = autoscaling.get_launch_configuration(autoscaling_group, region)

        return cls(cluster, capacity_provider, autoscaling_group, launch_configuration, region)

    @property
    def exists(self):
        """Indicate whether or not teardown of all members of the stack has begun.

        This is also not a particularly well-error-handled method.

        Returns:
            True iff the teardown process has at least begun for every member of the stack.
        """

        autoscaling_client = boto3.client("autoscaling", region_name=self.region)
        ecs_client = boto3.client("ecs", region_name=self.region)
        clusters = ecs_client.describe_clusters(clusters=(self.cluster,))["clusters"]
        capacity_providers = ecs_client.describe_capacity_providers(
            capacityProviders=(self.capacity_provider,)
        )["capacityProviders"]
        autoscaling_groups = autoscaling_client.describe_auto_scaling_groups(
            AutoScalingGroupNames=(self.autoscaling_group,)
        )["AutoScalingGroups"]
        launch_configurations = autoscaling_client.describe_launch_configurations(
            LaunchConfigurationNames=(self.launch_configuration,)
        )["LaunchConfigurations"]

        return (
            (clusters and clusters[0]["status"] not in ("DEPROVISIONING", "INACTIVE"))
            or (
                capacity_providers
                and capacity_providers[0]["updateStatus"]
                not in ("DELETE_COMPLETE", "DELETE_IN_PROGRESS")
            )
            or (autoscaling_groups and "Status" not in autoscaling_groups[0])
            or launch_configurations
        )


@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
@pytest.mark.parametrize("region", ("us-east-1",))
def test_lifecycle(region):
    """Test creation and deletion of an ECS cluster through Fractal's web API."""

    cluster_name = f"test-cluster-{uuid.uuid4()}"
    ecs_client = boto3.client("ecs", region_name=region)
    exception = None

    create_new_cluster.delay(
        cluster_name=cluster_name, region_name=region, max_size=0, ami=None
    ).get()

    # Make sure the cluster actually got created.
    cluster = ClusterInfo.query.filter_by(cluster=cluster_name, location=region).one()

    # Prevent poor error handling from breaking the test.
    try:
        stack = Stack.of(cluster_name, region)
    except Exception as exc:
        exception = exc

    delete_cluster.delay(cluster, force=False).get()
    db.session.expunge(cluster)

    with pytest.raises(NoResultFound):
        ClusterInfo.query.filter_by(cluster=cluster_name, location=region).one()

    # Prevent poor error handling from breaking the test.
    try:
        assert not stack.exists
    except Exception as exc:
        # Chain exceptions together manually.
        exc.__context__ = exception
        exception = exc

    if exception:
        raise exception


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
