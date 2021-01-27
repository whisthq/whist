import logging

from celery import shared_task

from app.helpers.utils.aws.base_ecs_client import ECSClient
from app.helpers.utils.general.logs import fractal_log
from app.models import (
    db,
    ClusterInfo,
    RegionToAmi,
)


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

    TODO: update RegionToAmi to use new AMI. We probably want a local testing
    db first.
    """
    self.update_state(
        state="PENDING",
        meta={
            "msg": (f"updating to ami {ami} in region {region_name}"),
        },
    )
    region_to_ami = RegionToAmi.query.filter_by(
        region_name=region_name,
    ).limit(1).first()

    if region_to_ami is None:
        raise ValueError(f"Region {region_name} is not in db.")

    if ami is None:
        # use existing AMI
        ami = region_to_ami.ami_id
    else:
        # update db with new AMI
        region_to_ami.ami_id = ami
        db.session.commit()

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

    region_name

    self.update_state(
        state="SUCCESS",
        meta={
            "msg": f"updated to ami {ami} in region {region_name}",
        },
    )
