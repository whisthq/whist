"""Unit and integration tests for code that we use to manipulate ECS clusters."""

import uuid

import boto3
import pytest

from app.celery.aws_ecs_creation import create_new_cluster
from app.models import ClusterInfo


@pytest.mark.usefixtures("authorized")
@pytest.mark.usefixtures("celery_worker")
@pytest.mark.parametrize("region", ("us-east-1",))
def test_lifecycle(celery_app, client, region):
    """Test creation and deletion of an ECS cluster through Fractal's web API."""

    cluster_name = f"test-cluster-{uuid.uuid4()}"
    ecs_client = boto3.client("ecs", region_name=region)

    create_new_cluster.delay(
        cluster_name=cluster_name, region_name=region, max_size=0, ami=None
    ).get()

    # Make sure the cluster actually got created.
    assert ClusterInfo.query.get(cluster_name) is not None
    assert ecs_client.describe_clusters(clusters=(cluster_name,))[
        "clusters"
    ], f"Failed to create {cluster_name} in {region}"

    response = client.post(
        "/aws_container/delete_cluster",
        json={
            "cluster_name": cluster_name,
            "region_name": region,
        },
    )

    assert response.status_code == 202

    # Block until the entire ECS stack has been torn down.
    celery_app.AsyncResult(response.get_json()["ID"]).get()

    # Make sure the cluster was actually deleted.
    clusters = ecs_client.describe_clusters(clusters=(cluster_name,))["clusters"]

    if len(clusters) > 0:
        assert clusters[0]["clusterName"] == cluster_name
        assert clusters[0]["status"] in ("DEPROVISIONING", "INACTIVE")

    assert ClusterInfo.query.get(cluster_name) is None
