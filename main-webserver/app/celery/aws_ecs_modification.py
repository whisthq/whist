import math
import logging

from celery import shared_task

from app.helpers.utils.aws.base_ecs_client import ECSClient
from app.helpers.utils.general.logs import fractal_log
from app.models import (
    db,
    ClusterInfo,
    RegionToAmi,
)
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
    self.update_state(
        state="PENDING",
        meta={
            "msg": (f"updating cluster {cluster_name} on ECS to ami {ami} in region {region_name}"),
        },
    )

    fractal_log(
        function="update_cluster",
        label="None",
        logs=f"updating cluster {cluster_name} on ECS to ami {ami} in region {region_name}",
    )

    all_regions = RegionToAmi.query.all()
    region_to_ami = {region.region_name: region.ami_id for region in all_regions}
    if ami is None:
        ami = region_to_ami[region_name]
    ecs_client = ECSClient(launch_type="EC2", region_name=region_name)
    ecs_client.update_cluster_with_new_ami(cluster_name, ami)

    self.update_state(
        state="SUCCESS",
        meta={
            "msg": (f"updating cluster {cluster_name} on ECS to ami {ami} in region {region_name}"),
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
    self.update_state(
        state="PENDING",
        meta={
            "msg": (f"updating to ami {ami} in region {region_name}"),
        },
    )

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
        fractal_log(
            "update_region",
            None,
            f"updated AMI in {region_name} to {ami}",
        )

    all_clusters = list(ClusterInfo.query.filter_by(location=region_name).all())
    all_clusters = [cluster for cluster in all_clusters if "cluster" in cluster.cluster]

    if len(all_clusters) == 0:
        fractal_log(
            function="update_region",
            label=None,
            logs=f"No clusters found in region {region_name}",
            level=logging.WARNING,
        )
        self.update_state(
            state="SUCCESS",
            meta={
                "msg": f"No clusters in region {region_name}",
            },
        )
        return

    fractal_log(
        function="update_region",
        label=None,
        logs=f"Updating clusters: {all_clusters}",
    )
    tasks = []
    for cluster in all_clusters:
        task = update_cluster.delay(region_name, cluster.cluster, ami)
        tasks.append(task.id)

    fractal_log(
        function="update_region",
        label=None,
        logs="update_region is returning with success. It spun up the "
        f"following update_cluster tasks: {tasks}",
    )

    self.update_state(
        state="SUCCESS",
        meta={
            "msg": f"updated to ami {ami} in region {region_name}",
        },
    )


@shared_task(bind=True)
def manual_scale_cluster(self, cluster: str, region_name: str):
    """
    Manually scales the cluster according the the following logic:
        - if num_tasks // FACTOR < num_instances, set_desired_capacity to x such that
        x < num_tasks * FACTOR < x + 1

    FACTOR is hard-coded to 10 at the moment.
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

    #TODO: set in environment or make configurable
    factor = 10

    ecs_client = ECSClient(launch_type="EC2", region_name=region_name)
    cluster_data = ecs_client.describe_cluster(cluster)
    keys = [
        "runningTasksCount",
        "pendingTasksCount",
        "registeredContainerInstancesCount",
    ]
    for k in keys:
        if k not in cluster_data:
            raise ValueError(f"Expected key {k} in AWS describle_cluster API response: {cluster_data}")
            
    tasks = cluster_data["runningTasksCount"] + cluster_data["pendingTasksCount"]
    instances = cluster_data["registeredContainerInstancesCount"]
    expected_instances = math.ceil(tasks/factor)

    if expected_instances < instances:
        # we only implement manual scale down
        self.update_state(
            state="PENDING",
            meta={
                "msg": f"Scaling cluster {cluster} from {instances} to {expected_instances}.",
            },
        )
        fractal_log(
            "manual_scale_cluster",
            None,
            f"""Cluster {cluster} had {instances} instances but should have had 
                {expected_instances}. Number of tasks: {tasks}. Triggering a scale down.""",
            level=logging.WARNING,
        )

        asg_list = ecs_client.get_auto_scaling_groups_in_cluster(cluster)
        if len(asg_list) != 1:
            #TODO: SENTRY
            raise ValueError(f"Expected 1 ASG but got {len(asg_list)} for cluster {cluster}")
        
        asg = asg_list[0]
        ecs_client.set_auto_scaling_group_capacity(asg, expected_instances)

        self.update_state(
            state="SUCCESS",
            meta={
                "msg": f"Scaled cluster {cluster} from {instances} to {expected_instances}.",
            },
        )

    else:
        self.update_state(
            state="SUCCESS",
            meta={
                "msg": f"Cluster {cluster} did not need any scaling.",
            },
        )
