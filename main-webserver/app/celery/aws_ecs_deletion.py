import time
import traceback

from app import (
    INTERNAL_SERVER_ERROR,
    REQUEST_TIMEOUT,
    SUCCESS,
    UNAUTHORIZED,
    celery_instance,
    db,
    fractalLog,
    fractalSQLCommit,
    fractalSQLUpdate,
    logging,
    fractalSQLUpdate,
    logging,
)
from app.helpers.utils.aws.aws_resource_locks import lockContainerAndUpdate, spinLock
from app.helpers.utils.aws.base_ecs_client import ECSClient
from app.serializers.hardware import ClusterInfo, UserContainer


@celery_instance.task(bind=True)
def deleteContainer(self, user_id, container_name):
    """

    Args:
        self: the celery instance running the task
        container_name (str): the ARN of the running container
        user_id (str): the user trying to delete the container

    Returns: json indicating success or failure

    """
    if spinLock(container_name) < 0:
        return {"status": REQUEST_TIMEOUT}
    container = UserContainer.query.get(container_name)
    if container.user_id != user_id:
        fractalLog(
            function="deleteContainer", label=str(container_name), logs="Wrong user",
        )
        self.update_state(
            state="FAILURE",
            meta={
                "msg": "Container {container_name} doesn't belong to user {user_id}".format(
                    container_name=container_name, user_id=user_id
                )
            },
        )
        return {"status": UNAUTHORIZED}
    fractalLog(
        function="deleteContainer",
        label=str(container_name),
        logs="Beginning to delete Container {container_name}. Goodbye, {container_name}!".format(
            container_name=container_name
        ),
    )

    lockContainerAndUpdate(
        container_name=container_name, state="DELETING", lock=True, temporary_lock=10
    )
    container_cluster = container.cluster
    ecs_client = ECSClient(base_cluster=container_cluster, grab_logs=False)
    ecs_client.add_task(container_name)
    try:
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
        fractalSQLCommit(db, lambda db, x: db.session.delete(x), container)

        cluster_info = ClusterInfo.query.get(container_cluster)
        if not cluster_info:
            fractalSQLCommit(
                db, lambda db, x: db.session.add(x), ClusterInfo(cluster=container_cluster)
            )
            cluster_info = ClusterInfo.query.filter_by(cluster=container_cluster).first()
        cluster_usage = ecs_client.get_clusters_usage(clusters=[container_cluster])[
            container_cluster
        ]
        cluster_sql = fractalSQLCommit(db, fractalSQLUpdate, cluster_info, cluster_usage)
        if cluster_sql:
            fractalLog(
                function="deleteContainer",
                label=container_name,
                logs=f"Removed task from cluster {container_cluster} and updated cluster info",
            )
            return {"status": SUCCESS}
        else:
            fractalLog(
                function="deleteContainer", label=container_name, logs="SQL insertion unsuccessful",
            )
            self.update_state(
                state="FAILURE",
                meta={"msg": "Error updating cluster {} in SQL".format(cluster=container_cluster)},
            )
            return None
    except Exception as e:
        fractalLog(
            function="deleteContainer",
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


@celery_instance.task(bind=True)
def drainContainer(self, container_name):
    if spinLock(container_name) < 0:
        return {"status": REQUEST_TIMEOUT}
    container = UserContainer.query.get(container_name)
    fractalLog(
        function="deleteContainer",
        label=str(container_name),
        logs="Beginning to drain Container {container_name}. Goodbye, {container_name}!".format(
            container_name=container_name
        ),
    )

    lockContainerAndUpdate(
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
        fractalLog(
            function="drainContainer",
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


@celery_instance.task(bind=True)
def delete_cluster(self, cluster, region_name):
    try:
        ecs_client = ECSClient(region_name=region_name)
        running_tasks = ecs_client.ecs_client.list_tasks(cluster=cluster, desiredStatus="RUNNING")[
            "taskArns"
        ]
        if running_tasks:
            fractalLog(
                function="delete_cluster",
                label=cluster,
                logs=f"Cannot delete cluster {cluster} with running tasks {running_tasks}. Please delete the tasks first.",
                level=logging.ERROR,
            )
            self.update_state(
                state="FAILURE", meta={"msg": f"Cannot delete clusters with running tasks"},
            )
        else:
            fractalLog(
                function="delete_cluster",
                label=cluster,
                logs="Deleting cluster {} in region {} and all associated instances".format(
                    cluster, region_name
                ),
            )
            ecs_client.terminate_containers_in_cluster(cluster)
            self.update_state(
                state="PENDING", meta={"msg": "Terminating containers in {}".format(cluster,)},
            )
            cluster_info = ClusterInfo.query.filter_by(cluster=cluster)
            fractalSQLCommit(db, lambda _, x: x.update({"status": "INACTIVE"}), cluster_info)
            ecs_client.spin_til_no_containers(cluster)
            ecs_client.ecs_client.delete_cluster(cluster=cluster)
            cluster_info = ClusterInfo.query.get(cluster)
            fractalSQLCommit(db, lambda db, x: db.session.delete(x), cluster_info)
    except Exception as error:
        traceback_str = "".join(traceback.format_tb(error.__traceback__))
        print(traceback_str)
        fractalLog(
            function="delete_cluster",
            label="None",
            logs=f"Encountered error: {error}, Traceback: {traceback_str}",
            level=logging.ERROR,
        )
        self.update_state(
            state="FAILURE", meta={"msg": f"Encountered error: {error}"},
        )
