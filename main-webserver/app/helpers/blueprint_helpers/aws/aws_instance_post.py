import threading
import time

from collections import defaultdict
from sys import maxsize
from typing import List, Optional
import requests
from flask import current_app
from app.models.hardware import (
    db,
    InstanceSorted,
    RegionToAmi,
    InstanceInfo,
    InstancesWithRoomForContainers,
)
from app.helpers.utils.db.db_utils import set_local_lock_timeout
from app.helpers.utils.aws.base_ec2_client import EC2Client
from app.helpers.utils.general.name_generation import generate_name
from app.constants.instance_state_values import PRE_CONNECTION, DRAINING

bundled_region = {
    "us-east-1": ["us-east-2"],
    "us-east-2": ["us-east-1"],
    "us-west-1": ["us-west-2"],
    "us-west-2": ["us-west-1"],
}


def find_instance(region: str, client_commit_hash: str) -> Optional[str]:
    """
    Given a region, finds (if it can) an instance in that region or a neighboring region with space
    If it succeeds, returns the instance ID.  Else, returns None
    Args:
        region: which region to search

    Returns: either a good instance ID or None

    """
    # 5sec arbitrarily decided as sufficient timeout when using with_for_update
    set_local_lock_timeout(5)
    avail_instance: Optional[InstanceSorted] = (
        InstanceSorted.query.filter_by(location=region, commit_hash=client_commit_hash)
        .limit(1)
        .with_for_update(skip_locked=True)
        .one_or_none()
    )
    if avail_instance is None:
        # check each replacement region for available containers
        for bundlable_region in bundled_region.get(region, []):
            # 5sec arbitrarily decided as sufficient timeout when using with_for_update
            set_local_lock_timeout(5)
            avail_instance = (
                InstanceSorted.query.filter_by(
                    location=bundlable_region, commit_hash=client_commit_hash
                )
                .limit(1)
                .with_for_update(skip_locked=True)
                .one_or_none()
            )
            if avail_instance is not None:
                break
    if avail_instance is None:
        return avail_instance
    return avail_instance.instance_name


def _get_num_new_instances(region: str, ami_id: str) -> int:
    """
    This function, given a region and an AMI ID, returns the number of new instances
    that we want to have spun up in that region with that AMI.  Returns
     -sys.maxsize for cases of "that AMI is inactive and we want to get rid of
     all instances running it when possible", since in that case
     we want to remove every instance of that type (and -sys.maxsize functions as
     negative infinity).

     At the moment, our scaling algorithm is
     - 'if we have less than 10 containers in a valid AMI/region pair,
     make a new instance'
     - 'Else, if we have a full instance of extra space more than 10, try to
     stop an instance'
     - 'Else, we're fine'.
    Args:
        region: which region we care about
        ami_id: which AMI ID we're checking

    Returns: the integer number of new instances we want live

    """

    # If the region is invalid or the AMI is not current, we want no buffer
    if region not in {x.region_name for x in RegionToAmi.query.all()}:
        return -maxsize
    active_ami_for_given_region = RegionToAmi.query.filter_by(
        region_name=region, ami_active=True
    ).one_or_none()
    if active_ami_for_given_region is None:
        return -maxsize
    if ami_id != active_ami_for_given_region.ami_id:
        return -maxsize
    # Now, we want to get the average number of containers per instance in that region
    # and the number of free containers
    all_instances = list(InstanceInfo.query.filter_by(location=region, aws_ami_id=ami_id).all())

    if len(all_instances) == 0:
        # If there are no instances running, we want one.
        return 1
    all_free_instances = list(
        InstancesWithRoomForContainers.query.filter_by(location=region, aws_ami_id=ami_id).all()
    )
    num_free_containers = sum(
        instance.container_capacity - instance.num_running_containers
        for instance in all_free_instances
    )
    avg_max_containers = sum(instance.container_capacity for instance in all_instances) / len(
        all_instances
    )

    # And then figure out how many instances we need to spin up/purge to get 10 free total
    desired_free_containers = 10.0

    if num_free_containers < desired_free_containers:
        return 1

    if num_free_containers >= (desired_free_containers + avg_max_containers):
        return -1

    return 0


# We make a collection of mutexes, one for each region and AMI.
# Defaultdict is a lazy dict, that creates default objects (in this case locks)
# as needed.
# Note that this only works if we have 1 web process,
# Multiprocess support will require DB synchronization/locking
scale_mutex = defaultdict(threading.Lock)


def do_scale_up_if_necessary(
    region: str, ami: str, force_buffer: Optional[int] = 0
) -> List[InstanceInfo]:
    """
    Scales up new instances as needed, given a region and AMI to check
    Specifically, if we want to add X instances (_get_num_new_instances
    returns X), we generate X new names and spin up new instances with those
    names.
    Args:
        region: which region to check for scaling
        ami: which AMI to scale with
        force_buffer: this will be used to override the recommendation for
        number of instances to be launched by `_get_num_new_instances`

    Returns: List of database objects representing the instances created.

    """
    new_instances = []
    with scale_mutex[f"{region}-{ami}"]:

        # num_new indicates how many new instances need to be spun up from
        # the ami that is passed in. Usually, we calculate that through
        # invoking `_get_num_new_instances` function. However, we can chose to
        # ignore that recommendation by passing in the `force_buffer` value.
        num_new = 0
        if force_buffer > 0:
            num_new = force_buffer
        else:
            num_new = _get_num_new_instances(region, ami)

        ami_obj = RegionToAmi.query.filter_by(region_name=region, ami_id=ami).one_or_none()

        instance_type = "g3.4xlarge"
        if num_new > 0:
            client = EC2Client(region_name=region)
            base_name = generate_name(starter_name=region)
            # TODO: test that we actually get 16 containers per instance
            # Which is savvy's guess as to g3.4xlarge capacity
            # TODO: Move this value to top-level config when more fleshed out
            base_number_free_containers = 16
            for index in range(num_new):
                instance_ids = client.start_instances(
                    image_id=ami,
                    instance_name=base_name + f"-{index}",
                    num_instances=1,
                    instance_type=instance_type,
                )
                # Setting last update time to -1 indicates that the instance
                # hasn't told the webserver it's live yet. We add the rows to
                # the DB now so that future scaling operations don't
                # double-scale.
                new_instance = InstanceInfo(
                    location=region,
                    aws_ami_id=ami,
                    cloud_provider_id=f"aws-{instance_ids[0]}",
                    instance_name=base_name + f"-{index}",
                    aws_instance_type=instance_type,
                    container_capacity=base_number_free_containers,
                    last_updated_utc_unix_ms=-1,
                    creation_time_utc_unix_ms=int(time.time()),
                    status=PRE_CONNECTION,
                    commit_hash=ami_obj.client_commit_hash,
                    auth_token="",  # Will be set through HTTP request `host_service` once it boots up.
                    ip="",  # Will be set through HTTP request `host_service` once it boots up.
                )
                new_instances.append(new_instance)
                db.session.add(new_instance)
                db.session.commit()
    return new_instances


def try_scale_down_if_necessary(region: str, ami: str) -> None:
    """
    Scales down new instances as needed, given a region and AMI to check
    Specifically, if we want to remove X instances (_get_num_new_instances
    returns -X), we get as many inactive instances as we can (up to X)
    and stop them.
    Args:
        region: which region to check for scaling
        ami: which AMI to scale with

    Returns: None

    """
    with scale_mutex[f"{region}-{ami}"]:
        num_new = _get_num_new_instances(region, ami)
        if num_new < 0:
            # we only want to scale down unused instances
            available_empty_instances = list(
                InstancesWithRoomForContainers.query.filter_by(
                    location=region, aws_ami_id=ami, num_running_containers=0
                )
                .limit(abs(num_new))
                .all()
            )
            if len(available_empty_instances) == 0:
                return
            for instance in available_empty_instances:
                # grab a lock on the instance to ensure nothing new's being assigned to it
                instance_info = InstanceInfo.query.with_for_update().get(instance.instance_name)
                instance_containers = InstancesWithRoomForContainers.query.filter_by(
                    instance_name=instance.instance_name
                ).one_or_none()
                if instance_containers is None or instance_containers.num_running_containers != 0:
                    db.session.commit()
                    continue
                instance_info.status = DRAINING
                try:
                    base_url = (
                        f"http://{instance_info.ip}:{current_app.config['HOST_SERVICE_PORT']}"
                    )
                    requests.post(f"{base_url}/drain_and_shutdown")
                except requests.exceptions.RequestException:
                    client = EC2Client(region_name=region)
                    client.stop_instances(
                        ["-".join(instance_info.cloud_provider_id.split("-")[1:])]
                    )
                db.session.commit()


def try_scale_down_if_necessary_all_regions() -> None:
    """
    Runs try_scale_down_if_necessary on every region/AMI pair in our db

    """
    region_and_ami_list = [
        (region.location, region.aws_ami_id)
        for region in InstanceInfo.query.distinct(
            InstanceInfo.location, InstanceInfo.aws_ami_id
        ).all()
    ]
    for region, ami in region_and_ami_list:
        try_scale_down_if_necessary(region, ami)


def repeated_scale_down_harness(time_delay: int, flask_app=current_app) -> None:
    """
    checks scaling every time_delay seconds.
    NOTE:  this function keeps looping and will
    not stop manually.
    Only run in background threads.

    Args:
        time_delay (int):  how often to run the scaling, in seconds
        flask_app (Flask.application):  app context, needed for DB operations
    """
    with flask_app.app_context():
        while True:
            try_scale_down_if_necessary_all_regions()
            time.sleep(time_delay)
