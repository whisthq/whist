import logging

import celery
from celery import shared_task, group

from app.helpers.utils.aws.base_ecs_client import ECSClient
from app.helpers.utils.general.logs import fractal_log
from app.models import (
    SortedClusters,
    RegionToAmi,
)
from app.helpers.utils.celery.celery_utils import find_group_task_with_state


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

    fractal_log(
        function="update_cluster",
        label="None",
        logs=f"updating cluster {cluster_name} on ECS to ami {ami} in region {region_name}",
    )
    self.update_state(
        state="PENDING",
        meta={
            "msg": (f"updating cluster {cluster_name} on ECS to ami {ami} in region {region_name}"),
        },
    )
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
    all_regions = RegionToAmi.query.all()
    region_to_ami = {region.region_name: region.ami_id for region in all_regions}
    if ami is None:
        ami = region_to_ami[region_name]
    all_clusters = list(SortedClusters.query.filter_by(location=region_name).all())
    all_clusters = [cluster for cluster in all_clusters if "cluster" in cluster.cluster]

    tasks = []
    for cluster in all_clusters:
        # .s sets up the task to be run with args but does not actually start running it
        task = update_cluster.s(region_name, cluster.cluster, ami)
        tasks.append(task)
    job = group(tasks)
    job_res = job.apply_async()

    success = False
    try:
        # give all tasks 60 seconds to finish
        job_res.get(timeout=60)
        success = True

    except celery.exceptions.TimeoutError:
        # a task did not finish
        bad_task_i = find_group_task_with_state(job_res, "PENDING")
        bad_cluster = all_clusters[bad_task_i]
        fractal_log(
            "update_region",
            None,
            f"Timed out when updating cluster {bad_cluster}",
            level=logging.ERROR,
        )

    except Exception as e:
        # a task experienced an error and is reraised here
        bad_task_i = find_group_task_with_state(job_res, "FAILURE")
        bad_cluster = all_clusters[bad_task_i]
        fractal_log(
            "update_region",
            None,
            f"Exception when updating cluster {bad_cluster}. Error str: {str(e)}",
            level=logging.ERROR,
        )

    if success:
        self.update_state(
            state="SUCCESS",
            meta={
                "msg": f"updated to ami {ami} in region {region_name}",
            },
        )
    else:
        self.update_state(
            state="FAILURE",
            meta={
                "msg": f"failed to update to ami {ami} in region {region_name}",
            },
        )
