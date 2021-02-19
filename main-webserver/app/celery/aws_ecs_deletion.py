import logging
import time

import boto3
import botocore

from celery import current_task, shared_task
from flask import current_app

from app.celery.aws_ecs_modification import manual_scale_cluster
from app.celery_utils import BenignError
from app.helpers.utils.aws.aws_resource_locks import (
    lock_container_and_update,
    spin_lock,
)
from app.helpers.utils.aws.base_ecs_client import ECSClient
from app.helpers.utils.aws.aws_resource_integrity import ensure_container_exists
from app.helpers.utils.datadog.events import (
    datadogEvent_containerDelete,
)
from app.helpers.utils.general.logs import fractal_log
from app.helpers.utils.general.sql_commands import (
    fractal_sql_commit,
    fractal_sql_update,
)
from app.models import ClusterInfo, db, UserContainer


@shared_task(bind=True)
def delete_container(self, container_name, aes_key):
    """Delete a container.

    Args:
        container_name (str): The ARN of the running container.
        aes_key (str): The 32-character AES key that the container uses to
            verify its identity.

    Returns:
        None
    """
    task_start_time = time.time()

    if spin_lock(container_name) < 0:
        fractal_log(
            function="delete_container",
            label=container_name,
            logs="spin_lock took too long.",
            level=logging.ERROR,
        )

        raise Exception("Failed to acquire resource lock.")

    try:
        container = ensure_container_exists(UserContainer.query.get(container_name))
    except Exception as error:
        message = f"Container {container_name} was not in the database."
        fractal_log(
            function="delete_container",
            label=container_name,
            logs=message,
            level=logging.ERROR,
        )
        raise Exception("The requested container does not exist in the database.") from error
    if not container:
        return

    if container.secret_key.lower() != aes_key.lower():
        message = f"Expected secret key {container.secret_key}. Got {aes_key}."

        fractal_log(
            function="delete_container",
            label=container_name,
            logs=message,
            level=logging.ERROR,
        )

        raise Exception("The requested container does not exist.")

    fractal_log(
        function="delete_container",
        label=str(container_name),
        logs="Beginning to delete container {container_name}. Goodbye!".format(
            container_name=container_name
        ),
    )

    lock_container_and_update(
        container_name=container_name, state="DELETING", lock=True, temporary_lock=10
    )

    container_cluster = container.cluster
    container_location = container.location
    ecs_client = ECSClient(
        base_cluster=container_cluster, region_name=container_location, grab_logs=False
    )

    ecs_client.add_task(container_name)

    # TODO: just pass task as param
    if not ecs_client.check_if_done(offset=0):
        ecs_client.stop_task(reason="API triggered task stoppage", offset=0)
        self.update_state(
            state="PENDING",
            meta={
                "msg": "Container {container_name} begun stoppage".format(
                    container_name=container_name,
                )
            },
        )
        ecs_client.spin_til_done(offset=0)
    fractal_sql_commit(db, lambda db, x: db.session.delete(x), container)

    cluster_info = ClusterInfo.query.get(container_cluster)
    if not cluster_info:
        fractal_sql_commit(
            db, lambda db, x: db.session.add(x), ClusterInfo(cluster=container_cluster)
        )
        cluster_info = ClusterInfo.query.filter_by(cluster=container_cluster).first()
    cluster_usage = ecs_client.get_clusters_usage(clusters=[container_cluster])[container_cluster]
    cluster_sql = fractal_sql_commit(db, fractal_sql_update, cluster_info, cluster_usage)
    if cluster_sql:
        fractal_log(
            function="delete_container",
            label=container_name,
            logs=f"Removed task from cluster {container_cluster} and updated cluster info",
        )
    else:
        fractal_log(
            function="delete_container",
            label=container_name,
            logs="Failed to update cluster resources.",
        )

        raise Exception("SQL update failed.")

    # trigger celery task to see if manual scaling should be done
    manual_scale_cluster.delay(container_cluster, container_location)

    if not current_app.testing:
        task_time_taken = time.time() - task_start_time
        try:
            datadogEvent_containerDelete(
                container_name, container_cluster, lifecycle=True, time_taken=task_time_taken
            )
        except KeyError:
            pass


@shared_task
def deregister_container_instances(cluster_name, region):
    """Deregister all ECS container instances from the specified ECS cluster.

    This Celery task will forcibly stop all ECS tasks that are currently running on the cluster
    whose container instances are to be deregistered.

    Args:
        cluster_name: The short name of the cluster to delete as a string.
        region: The region in which the cluster is located as a string (e.g. "us-east-1").

    Returns:
        None
    """

    ecs_client = boto3.client("ecs", region_name=region)
    pages = ecs_client.get_paginator("list_container_instances").paginate(cluster=cluster_name)

    try:
        instances = tuple(arn for page in pages for arn in page["containerInstanceArns"])
    except ecs_client.exceptions.ClusterNotFoundException as error:
        raise BenignError(error)  # pylint: disable=raise-missing-from

    else:
        for instance in instances:
            ecs_client.deregister_container_instance(
                cluster=cluster_name, containerInstance=instance, force=True
            )


@shared_task
def delete_cluster(cluster_name, region):
    """Delete the specified ECS cluster.

    This Celery task will fail if there are any container instances that are still registered to
    the cluster.

    Args:
        cluster_name: The short name of the cluster to delete as a string.
        region: The region in which the cluster is located as a string (e.g. "us-east-1").

    Returns:
        A list of strings representing the short names of capacity providers that were attached to
        the cluster before it was deleted.
    """

    ecs_client = boto3.client("ecs", region_name=region)

    try:
        response = ecs_client.delete_cluster(cluster=cluster_name)
    except botocore.exceptions.ClientError as error:
        if error.response["Error"]["Code"] == "ClusterNotFoundException":
            # The cluster doesn't exist and the task has failed, but the failure is not a cause
            # for concern because the cluster pointer was removed from the database before the
            # ECS cluster deletion pipeline was initiated.
            raise BenignError(error)  # pylint: disable=raise-missing-from

        if error.response["Error"]["Code"] == "UpdateInProgressException":
            # Try again in a few seconds after the update has completed.
            current_task.retry(countdown=5, max_retries=5)

        raise error
    else:
        capacity_providers = response["cluster"]["capacityProviders"]

    return capacity_providers


@shared_task
def delete_capacity_providers(capacity_providers, region):
    """Delete the specified capacity providers.

    All of the specified capacity providers must be auto-scaling capacity providers.

    Args:
        capacity_providers: A list of strings representing the names of the capacity providers to
            be deleted.
        region: The AWS region in which the capacity providers are located (e.g. "us-east-1").

    Returns:
        A list of strings representing the ARNs of each capacity provider's underlying auto-scaling
        group.
    """

    ecs_client = boto3.client("ecs", region_name=region)
    auto_scaling_groups = []

    for provider in capacity_providers:
        try:
            response = ecs_client.delete_capacity_provider(capacityProvider=provider)
        except botocore.exceptions.ClientError:
            # Skip any capacity providers that cause errors.
            continue

        arn = response["capacityProvider"]["autoScalingGroupProvider"]["autoScalingGroupArn"]
        _, name = arn.rsplit("/", maxsplit=1)
        auto_scaling_groups.append(name)

    return auto_scaling_groups


@shared_task
def delete_auto_scaling_groups(auto_scaling_groups, region):
    """Delete the specified auto-scaling groups.

    All of the specified auto-scaling groups must use launch configurations instead of launch
    templates.

    Args:
        auto_scaling_groups: A list of strings representing the ARNs of the auto-scaling groups to
            be deleted.
        region: The AWS region in which all of the auto-scaling groups are located (e.g.
            "us-east-1").

    Returns:
        None
    """

    autoscaling_client = boto3.client("autoscaling", region_name=region)

    for asg in auto_scaling_groups:
        autoscaling_client.delete_auto_scaling_group(
            AutoScalingGroupName=asg,
            ForceDelete=True,
        )
