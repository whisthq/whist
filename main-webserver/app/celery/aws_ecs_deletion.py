import time

from typing import Union

from celery import Task, shared_task
from flask import current_app

from app.celery.aws_ecs_modification import manual_scale_cluster
from app.exceptions import ClusterNotIdle

from app.helpers.utils.aws.base_ecs_client import ECSClient
from app.helpers.utils.aws.aws_resource_integrity import ensure_container_exists
from app.helpers.utils.db.db_utils import set_local_lock_timeout
from app.helpers.utils.general.logs import fractal_logger
from app.helpers.utils.event_logging.events import (
    logged_event_container_deleted,
)
from app.helpers.utils.general.sql_commands import (
    fractal_sql_commit,
    fractal_sql_update,
)
from app.models import ClusterInfo, db, SortedClusters, UserContainer


@shared_task(bind=True)
def delete_container(self: Task, container_name: str, aes_key: str) -> None:
    """Delete a container.

    Args:
        self: the Celery instance executing the task
        container_name (str): The ARN of the running container.
        aes_key (str): The 32-character AES key that the container uses to
            verify its identity.

    Returns:
        None
    """
    task_start_time = time.time()
    # 30sec arbitrarily decided as sufficient timeout when using with_for_update
    set_local_lock_timeout(30)
    try:
        # lock using with_for_update()
        container = ensure_container_exists(
            UserContainer.query.with_for_update().get(container_name)
        )
    except Exception as error:
        message = f"Container {container_name} was not in the database."
        fractal_logger.error(
            message,
            extra={
                "label": container_name,
            },
        )
        raise Exception("The requested container does not exist in the database.") from error
    if not container:
        return

    if container.secret_key.lower() != aes_key.lower():
        message = f"Expected secret key {container.secret_key}. Got {aes_key}."
        fractal_logger.error(message, extra={"label": container_name})
        raise Exception("The requested container does not exist.")

    fractal_logger.info(
        "Beginning to delete container {container_name}. Goodbye!".format(
            container_name=container_name
        ),
        extra={"label": str(container_name)},
    )

    container_cluster = container.cluster
    container_user = container.user_id
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

    db.session.delete(container)
    db.session.commit()

    cluster_info = ClusterInfo.query.get(container_cluster)
    if not cluster_info:
        fractal_sql_commit(
            db, lambda db, x: db.session.add(x), ClusterInfo(cluster=container_cluster)
        )
        cluster_info = ClusterInfo.query.filter_by(cluster=container_cluster).first()
    cluster_usage = ecs_client.get_clusters_usage(clusters=[container_cluster])[container_cluster]
    cluster_sql = fractal_sql_commit(db, fractal_sql_update, cluster_info, cluster_usage)
    if cluster_sql:
        fractal_logger.info(
            f"Removed task from cluster {container_cluster} and updated cluster info",
            extra={"label": container_name},
        )
    else:
        fractal_logger.info("Failed to update cluster resources.", extra={"label": container_name})
        raise Exception("SQL update failed.")

    # trigger celery task to see if manual scaling should be done
    manual_scale_cluster.delay(container_cluster, container_location)

    if not current_app.testing:
        task_time_taken = int(time.time() - task_start_time)
        try:
            logged_event_container_deleted(
                container_name, container_user, container_cluster, time_taken=task_time_taken
            )
        except KeyError:
            pass


@shared_task(throws=(ClusterNotIdle,))
def delete_cluster(cluster: Union[ClusterInfo, SortedClusters], *, force: bool) -> None:
    """Tear down the specified cluster's cloud infrastructure and remove it from the database.

    This Celery task is responsible for updating the cluster pointer in the database appropriately.
    It initially deletes the row from the hardware.user_containers table, but later rolls that
    deletion back if anything goes wrong while the cloud stack is being torn down.
    ECSClient.delete_cluster() is responsible for making AWS queries.

    All arguments following the * in the argument list must be supplied as keyword arguments, even
    if they are not assigned defaults. This decision was made for two reasons:

    1. force=True and force=False are much more descriptive than just True and False and
    2. a default value for the force keyword argument is already chosen by the view function,
        which is this Celery task's only callee, that processes POST /aws_container/delete_cluster
        requests. If a "force" key is not supplied in the request's POST body, the view function
        automatically calls delete_cluster.delay() with force=False. Setting the default value in
        two different places would be redundant.

    Args:
        cluster: An instance of the ClusterInfo model representing the cluster to delete. The
            object must be in the persistent state (see below).
        force: A boolean indicating whether or not to delete the cluster even if there are still
            database records of tasks running on it.

    Returns:
        None

    Raises:
        ClusterNotIdle: There exist database records of Fractalized applications running on the
            specified the database and the force option is falsy.
    """

    # The cluster object with which this task is called must be in the persistent state. Otherwise,
    # load=False will cause the following line to raise an exception. Read more at
    # https://docs.sqlalchemy.org/en/13/orm/session_api.html#sqlalchemy.orm.session.Session.merge.params.load
    cluster_to_delete = db.session.merge(cluster, load=False)

    if cluster_to_delete.containers.count() > 0 and not force:
        raise ClusterNotIdle(cluster_to_delete.cluster, cluster_to_delete.location)

    db.session.delete(cluster_to_delete)
    db.session.flush()

    try:
        ECSClient.delete_cluster(cluster_to_delete.cluster, cluster_to_delete.location)
    except:
        # Reinstate the deleted row if any of our cloud API requests fail.
        db.session.rollback()
        raise

    db.session.commit()
