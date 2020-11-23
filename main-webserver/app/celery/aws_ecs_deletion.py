import logging
import traceback
import time

from celery import shared_task
from flask import current_app

from app.constants.http_codes import (
    INTERNAL_SERVER_ERROR,
    REQUEST_TIMEOUT,
    SUCCESS,
)
from app.helpers.utils.aws.aws_resource_locks import (
    lock_container_and_update,
    spin_lock,
)
from app.helpers.utils.aws.base_ecs_client import ECSClient
from app.helpers.utils.general.logs import fractal_log
from app.helpers.utils.general.sql_commands import (
    fractal_sql_commit,
    fractal_sql_update,
)
from app.models import db
from app.serializers.hardware import ClusterInfo, UserContainer

from app.helpers.utils.datadog.events import (
    datadogEvent_containerDelete,
    datadogEvent_clusterDelete,
)


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

    container = UserContainer.query.get(container_name)

    if not container or container.secret_key.lower() != aes_key.lower():
        if container:
            message = f"Expected secret key {container.secret_key}. Got {aes_key}."
        else:
            message = f"Container {container_name} does not exist."

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
        logs="Beginning to delete Container {container_name}. Goodbye, {container_name}!".format(
            container_name=container_name
        ),
    )

    lock_container_and_update(
        container_name=container_name, state="DELETING", lock=True, temporary_lock=10
    )

    container_cluster = container.cluster
    ecs_client = ECSClient(
        base_cluster=container_cluster, region_name=container.location, grab_logs=False
    )

    ecs_client.add_task(container_name)

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

    if not current_app.testing:
        task_time_taken = time.time() - task_start_time
        datadogEvent_containerDelete(
            container_name, container_cluster, lifecycle=True, time_taken=task_time_taken
        )


@shared_task(bind=True)
def drain_container(self, container_name):
    if spin_lock(container_name) < 0:
        return {"status": REQUEST_TIMEOUT}
    container = UserContainer.query.get(container_name)
    fractal_log(
        function="delete_container",
        label=str(container_name),
        logs="Beginning to drain Container {container_name}. Goodbye, {container_name}!".format(
            container_name=container_name
        ),
    )

    lock_container_and_update(
        container_name=container_name, state="DRAINING", lock=True, temporary_lock=10
    )
    container_cluster = container.cluster
    ecs_client = ECSClient(base_cluster=container_cluster, grab_logs=False)
    try:
        container_arn = ecs_client.get_container_for_tasks(
            [container_name], cluster=container_cluster
        )
        ecs_client.set_containers_to_draining([container_arn], cluster=container_cluster)
    except Exception as e:
        fractal_log(
            function="drain_container",
            label=str(container_name),
            logs="ran into deletion error {}".format(e),
        )

        self.update_state(
            state="FAILURE",
            meta={
                "msg": "Error stopping Container {container_name}".format(
                    container_name=container_name
                )
            },
        )
        return {"status": INTERNAL_SERVER_ERROR}
    return {"status": SUCCESS}


@shared_task(bind=True)
def delete_cluster(self, cluster, region_name):
    task_start_time = time.time()

    try:
        ecs_client = ECSClient(region_name=region_name)
        running_tasks = ecs_client.ecs_client.list_tasks(cluster=cluster, desiredStatus="RUNNING")[
            "taskArns"
        ]
        if running_tasks:
            fractal_log(
                function="delete_cluster",
                label=cluster,
                logs=(
                    f"Cannot delete cluster {cluster} with running tasks {running_tasks}. Please "
                    "delete the tasks first."
                ),
                level=logging.ERROR,
            )
            self.update_state(
                state="FAILURE",
                meta={"msg": "Cannot delete clusters with running tasks"},
            )
        else:
            fractal_log(
                function="delete_cluster",
                label=cluster,
                logs="Deleting cluster {} in region {} and all associated instances".format(
                    cluster, region_name
                ),
            )
            ecs_client.terminate_containers_in_cluster(cluster)
            self.update_state(
                state="PENDING",
                meta={
                    "msg": "Terminating containers in {}".format(
                        cluster,
                    )
                },
            )
            cluster_info = ClusterInfo.query.filter_by(cluster=cluster)
            fractal_sql_commit(db, lambda _, x: x.update({"status": "INACTIVE"}), cluster_info)
            ecs_client.spin_til_no_containers(cluster)

            asg_name = ecs_client.describe_auto_scaling_groups_in_cluster(cluster)[0][
                "AutoScalingGroupName"
            ]
            cluster_info = ClusterInfo.query.get(cluster)

            fractal_sql_commit(db, lambda db, x: db.session.delete(x), cluster_info)
            ecs_client.auto_scaling_client.delete_auto_scaling_group(
                AutoScalingGroupName=asg_name, ForceDelete=True
            )
            ecs_client.ecs_client.delete_cluster(cluster=cluster)
    except Exception as error:
        traceback_str = "".join(traceback.format_tb(error.__traceback__))
        print(traceback_str)
        fractal_log(
            function="delete_cluster",
            label="None",
            logs=f"Encountered error: {error}, Traceback: {traceback_str}",
            level=logging.ERROR,
        )
        self.update_state(
            state="FAILURE",
            meta={"msg": f"Encountered error: {error}"},
        )
    if not current_app.testing:
        task_time_taken = time.time() - task_start_time
        datadogEvent_clusterDelete(cluster, lifecycle=True, time_taken=task_time_taken)
