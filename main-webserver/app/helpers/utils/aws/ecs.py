"""Low-level helper functions that interact directly with AWS ECS."""

import boto3
import botocore

from .utils import Retry, retry_with_backoff


def delete_capacity_provider(capacity_provider, region):
    """Delete the specified capacity provider.

    The specified capacity provider must be an auto-scaling capacity provider.

    Args:
        capacity_provider: The short name of the capacity provider to delete as a string.
        region: The AWS region in which the capacity providers are located (e.g. "us-east-1").

    Returns:
        The short name of the capacity provider's underlying autoscaling group as a string or None
        if the capacity provider doesn't exist.
    """

    ecs_client = boto3.client("ecs", region_name=region)

    try:
        response = ecs_client.delete_capacity_provider(capacityProvider=capacity_provider)
    except ecs_client.exceptions.ClientException:
        # The capacity provider doesn't exist, but that's no big deal.
        name = None
    else:
        arn = response["capacityProvider"]["autoScalingGroupProvider"]["autoScalingGroupArn"]
        _, name = arn.rsplit("/", maxsplit=1)

    return name


def delete_cluster(cluster_name, region):
    """Delete the specified ECS cluster.

    This function will fail if there are still tasks pending or running on this cluster or if there
    are container instances registered to the cluster.

    Args:
        cluster_name: The short name of the cluster to delete as a string.
        region: The region in which the cluster is located as a string (e.g. "us-east-1").

    Returns:
        A list of strings representing the short names of the capacity providers that were attached
        to the cluster before it was deleted.
    """

    ecs_client = boto3.client("ecs", region_name=region)

    @retry_with_backoff(min_wait=1, wait_delta=2, max_wait=5)
    def _delete_cluster():
        try:
            response = ecs_client.delete_cluster(cluster=cluster_name)
        except botocore.exceptions.ClientError as error:
            if error.response["Error"]["Code"] == "UpdateInProgressException":
                raise Retry from error

            if error.response["Error"]["Code"] == "ClusterNotFoundException":
                # It's not a big deal if the cluster doesn't exist. If so, we just don't know which
                # capacity providers to delete, so we return an empty list.
                capacity_providers = []
            else:
                raise
        else:
            capacity_providers = response["cluster"]["capacityProviders"]

        return capacity_providers

    return _delete_cluster()


def deregister_container_instances(cluster_name, region):
    """Deregister all ECS container instances from the specified ECS cluster.

    This Celery task will forcibly stop all ECS tasks that are currently running on the cluster
    whose container instances are to be deregistered.

    Args:
        cluster_name: The short name of the cluster whose container instances are to be
            deregistered as a string.
        region: The region in which the cluster is located as a string (e.g. "us-east-1").

    Returns:
        None
    """

    ecs_client = boto3.client("ecs", region_name=region)
    pages = ecs_client.get_paginator("list_container_instances").paginate(cluster=cluster_name)

    try:
        instances = tuple(arn for page in pages for arn in page["containerInstanceArns"])
    except ecs_client.exceptions.ClusterNotFoundException:
        # It's not a big deal if the cluster has already been deleted.
        pass
    else:
        for instance in instances:
            try:
                ecs_client.deregister_container_instance(
                    cluster=cluster_name, containerInstance=instance, force=True
                )
            except ecs_client.exceptions.InvalidParameterException:
                # Don't freak out if the container instance doesn't exist.
                pass
