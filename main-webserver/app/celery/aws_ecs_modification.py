import math
import logging

from celery import shared_task
from flask import current_app

from app.helpers.utils.aws.base_ecs_client import ECSClient, FractalECSClusterNotFoundException
from app.helpers.utils.aws.aws_resource_integrity import ensure_container_exists
from app.helpers.utils.aws.aws_resource_locks import (
    lock_container_and_update,
    spin_lock,
)
from app.helpers.utils.general.logs import fractal_logger
from app.models import db, ClusterInfo, RegionToAmi, UserContainer
from app.helpers.utils.general.sql_commands import fractal_sql_commit


@shared_task(bind=True)
def update_cluster(self, region_name="us-east-1", cluster_name=None, ami=None):
    """
    Updates a specific cluster to use a new AMI
    :param self (CeleryInstance): the celery instance running the task
    :param region_name (str): which region the cluster is in
    :param cluster_name (str): which cluster to update
    :param ami (str): which AMI to use
    :return: which cluster was updated
    """
    all_regions = RegionToAmi.query.all()
    region_to_ami = {region.region_name: region.ami_id for region in all_regions}
    if ami is None:
        ami = region_to_ami[region_name]

    self.update_state(
        state="PENDING",
        meta={
            "msg": (f"updating cluster {cluster_name} on ECS to ami {ami} in region {region_name}"),
        },
    )
    fractal_logger.info(
        f"updating cluster {cluster_name} on ECS to ami {ami} in region {region_name}"
    )

    # We must delete every unassigned container in the cluster.
    unassigned_containers = UserContainer.query.filter_by(
        cluster=cluster_name, is_assigned=False
    ).all()
    for container in unassigned_containers:
        container_name = container.container_id

        # Lock the container in the database for a short amount of time.
        # This is to make sure that the container is not assigned while we are trying to delete it.
        if spin_lock(container_name) < 0:
            fractal_logger.error("spin_lock took too long.", extra={"label": container_name})
            raise Exception("Failed to acquire resource lock.")

        fractal_logger.info(
            "Beginning to delete unassigned container {container_name}. Goodbye!".format(
                container_name=container_name
            ),
            extra={"label": str(container_name)},
        )

        # Set the container state to "DELETING".
        # Lock it for 10 minutes (again, spin_lock is only a temporary lock.)
        lock_container_and_update(
            container_name=container_name, state="DELETING", lock=True, temporary_lock=10
        )

        container = ensure_container_exists(container)
        if not container:
            # The container never existed, and has been deleted!
            continue

        # Initialize ECSClient
        container_cluster = container.cluster
        container_location = container.location

        # Database constraints ensure that we will only reach here if the cluster exists,
        # because otherwise no containers would be found in the DB.
        ecs_client = ECSClient(
            base_cluster=container_cluster, region_name=container_location, grab_logs=False
        )

        # Update ecs_client to use the current task
        ecs_client.add_task(container_name)

        # Actually stop the task if it's not done.
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

        # Delete the row for this container from the database.
        fractal_sql_commit(db, lambda db, x: db.session.delete(x), container)

    # Check if a cluster does not exist.
    ecs_client = ECSClient(launch_type="EC2", region_name=region_name)

    try:
        ecs_client.update_cluster_with_new_ami(cluster_name, ami)
    except FractalECSClusterNotFoundException:
        # We should remove any entries in the DB referencing this cluster, as they are out of date
        # Cluster is a primary key, so `.first()` suffices
        bad_entry = ClusterInfo.query.filter_by(cluster=cluster_name).first()
        fractal_sql_commit(db, lambda db, x: db.session.delete(x), bad_entry)

        # Task was executed successfully; we have recorded the fact that this cluster did not exist.
        self.update_state(
            state="SUCCESS",
            meta={
                "msg": f"Cluster {cluster_name} in region {region_name} did not exist.",
            },
        )
    else:
        # The cluster did exist, and was updated successfully.
        self.update_state(
            state="SUCCESS",
            meta={
                "msg": f"updated cluster {cluster_name} to ami {ami} in region {region_name}",
            },
        )


@shared_task(bind=True)
def update_region(self, region_name="us-east-1", ami=None):
    """
    Updates all clusters in a region to use a new AMI
    calls update_cluster under the hood
    :param self (CeleryInstance): the celery instance running the task
    :param region_name (str): which region the cluster is in
    :param ami (str): which AMI to use
    :return: which cluster was updated
    """
    region_to_ami = RegionToAmi.query.filter_by(
        region_name=region_name,
    ).first()
    if region_to_ami is None:
        raise ValueError(f"Region {region_name} is not in db.")

    if ami is None:
        # use existing AMI
        ami = region_to_ami.ami_id
    else:
        # # update db with new AMI
        region_to_ami.ami_id = ami
        fractal_sql_commit(db)
        fractal_logger.info(
            f"updated AMI in {region_name} to {ami}",
        )

    self.update_state(
        state="PENDING",
        meta={
            "msg": (f"updating to ami {ami} in region {region_name}"),
        },
    )

    all_clusters = list(ClusterInfo.query.filter_by(location=region_name).all())
    all_clusters = [cluster for cluster in all_clusters if "cluster" in cluster.cluster]

    if len(all_clusters) == 0:
        fractal_logger.warning(
            f"No clusters found in region {region_name}",
        )
        self.update_state(
            state="SUCCESS",
            meta={"msg": f"No clusters in region {region_name}", "tasks": ""},
        )
        return

    fractal_logger.info(f"Updating clusters: {all_clusters}")
    tasks = []
    for cluster in all_clusters:
        task = update_cluster.delay(region_name, cluster.cluster, ami)
        tasks.append(task.id)

    fractal_logger.info(
        "update_region is returning with success. It spun up the "
        f"following update_cluster tasks: {tasks}"
    )

    # format tasks as space separated strings (used by AMI creation workflow to poll for success)
    delim = " "
    formatted_tasks = delim.join(tasks)

    self.update_state(
        state="SUCCESS",
        meta={"msg": f"updated to ami {ami} in region {region_name}", "tasks": formatted_tasks},
    )


@shared_task(bind=True)
def manual_scale_cluster(self, cluster: str, region_name: str):
    """
    Manually scales the cluster according the the following logic:
        1. see if active_tasks + pending_tasks > num_instances * AWS_TASKS_PER_INSTANCE. if so,
            we should trigger a scale down.
        2. check if there are instances with 0 tasks. sometimes AWS can suboptimally distribute
            our workloads, so we want to make sure there are instances we can actually delete.
            If AWS does poorly distribute our loads, we log it.
        3. trigger a scale down so only instances with tasks remain.
    This function does not handle outscaling at the moment.
    This function can be expanded to handle more custom scaling logic as we grow.
    Args:
        cluster: cluster to manually scale
        region_name: region that cluster resides in
    """
    self.update_state(
        state="PENDING",
        meta={
            "msg": f"Checking if cluster {cluster} should be scaled.",
        },
    )

    factor = int(current_app.config["AWS_TASKS_PER_INSTANCE"])

    ecs_client = ECSClient(launch_type="EC2", region_name=region_name)
    cluster_data = ecs_client.describe_cluster(cluster)
    keys = [
        "runningTasksCount",
        "pendingTasksCount",
        "registeredContainerInstancesCount",
    ]
    for k in keys:
        if k not in cluster_data:
            raise ValueError(
                f"Expected key {k} in AWS describle_cluster API response. Got: {cluster_data}"
            )

    num_tasks = cluster_data["runningTasksCount"] + cluster_data["pendingTasksCount"]
    num_instances = cluster_data["registeredContainerInstancesCount"]
    expected_num_instances = math.ceil(num_tasks / factor)

    if expected_num_instances == num_instances:
        self.update_state(
            state="SUCCESS",
            meta={
                "msg": f"Cluster {cluster} did not need any scaling.",
            },
        )
        return

    # now we check if there are actually empty instances
    instances = ecs_client.list_container_instances(cluster)
    instances_tasks = ecs_client.describe_container_instances(cluster, instances)

    empty_instances = 0
    for instance_task_data in instances_tasks:
        if "runningTasksCount" not in instance_task_data:
            msg = (
                "Expected key runningTasksCount in AWS describe_container_instances response."
                f" Got: {instance_task_data}"
            )
            raise ValueError(msg)
        rtc = instance_task_data["runningTasksCount"]
        if rtc == 0:
            empty_instances += 1

    if empty_instances == 0:
        msg = (
            f"Cluster {cluster} had {num_instances} instances but should have"
            f" {expected_num_instances}. Number of total tasks: {num_tasks}. However, no instance"
            " is empty so a scale down cannot be triggered. This means AWS ECS has suboptimally"
            " distributed tasks onto instances."
        )
        fractal_logger.info(msg)

        self.update_state(
            state="SUCCESS",
            meta={
                "msg": "Could not scale-down because no empty instances.",
            },
        )

    else:
        # at this point, we have detected scale-down should happen and know that
        # there are some empty instances for us to delete. we can safely do a scale-down.
        asg_list = ecs_client.get_auto_scaling_groups_in_cluster(cluster)
        if len(asg_list) != 1:
            raise ValueError(f"Expected 1 ASG but got {len(asg_list)} for cluster {cluster}")

        asg = asg_list[0]
        # keep whichever instances are not empty
        desired_capacity = num_instances - empty_instances
        ecs_client.set_auto_scaling_group_capacity(asg, desired_capacity)

        self.update_state(
            state="SUCCESS",
            meta={
                "msg": f"Scaled cluster {cluster} from {num_instances} to {desired_capacity}.",
            },
        )
