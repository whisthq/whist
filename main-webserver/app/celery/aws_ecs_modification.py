from celery import shared_task

from app.helpers.utils.aws.base_ecs_client import ECSClient
from app.helpers.utils.general.logs import fractal_log
from app.models import (
    SortedClusters,
    RegionToAmi,
)


@shared_task(bind=True)
def update_cluster(self, region_name="us-east-1", cluster_name=None, ami=None):
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
    for cluster in all_clusters:
        update_cluster.delay(region_name, cluster.cluster, ami)
    self.update_state(
        state="SUCCESS",
        meta={
            "msg": f"updating to ami {ami} in region {region_name}",
        },
    )
