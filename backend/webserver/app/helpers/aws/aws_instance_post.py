import threading
import time

from sys import maxsize
from collections import defaultdict
from typing import Any, DefaultDict, List, Optional, Union
from flask import current_app
from botocore.exceptions import ClientError
from app.database.models.cloud import (
    db,
    RegionToAmi,
    InstanceInfo,
    InstancesWithRoomForMandelboxes,
    MandelboxHostState,
)
from app.utils.db.db_utils import set_local_lock_timeout
from app.utils.aws.base_ec2_client import EC2Client
from app.utils.general.name_generation import generate_name
from app.utils.general.logs import whist_logger
from app.constants.mandelbox_assign_error_names import MandelboxAssignError

bundled_region = {
    "us-east-1": ["us-east-2", "ca-central-1"],
    "us-east-2": ["us-east-1", "ca-central-1"],
    "us-west-1": ["us-west-2"],
    "us-west-2": ["us-west-1"],
    "ca-central-1": ["us-east-1", "us-east-2"],
}

# The number of GPUs on each instance type
instance_type_to_gpu_map = {
    "g4dn.xlarge": 1,
    "g4dn.2xlarge": 1,
    "g4dn.4xlarge": 1,
    "g4dn.8xlarge": 1,
    "g4dn.16xlarge": 1,
    "g4dn.12xlarge": 4,
}

# The number of vCPUs on each instance type
instance_type_to_vcpu_map = {
    "g4dn.xlarge": 4,
    "g4dn.2xlarge": 8,
    "g4dn.4xlarge": 16,
    "g4dn.8xlarge": 32,
    "g4dn.16xlarge": 64,
    "g4dn.12xlarge": 48,
}

# We want to have multiple attempts at starting an instance
# if it fails due to insufficient capacity.
WAIT_TIME_BEFORE_RETRY_IN_SECONDS = 15
MAX_WAIT_TIME_IN_SECONDS = 200
MAX_RETRY_ATTEMPTS = MAX_WAIT_TIME_IN_SECONDS / WAIT_TIME_BEFORE_RETRY_IN_SECONDS

# Currently the limiting factors are GPU count and
# vCPU count. Our two constraints are mandelboxes/GPU
# (configured to 2 in the host service) and vCPUs/mandelbox
# (which we set to 4 here). This determines a very simple
# allocation setup that we can flesh out later.

HOST_SERVICE_MANDELBOXES_PER_GPU = 3
VCPUS_PER_MANDELBOX = 4

type_to_number_map = {
    k: min(
        instance_type_to_gpu_map[k] * HOST_SERVICE_MANDELBOXES_PER_GPU,
        instance_type_to_vcpu_map[k] // VCPUS_PER_MANDELBOX,
    )
    for k in instance_type_to_gpu_map.keys() & instance_type_to_vcpu_map.keys()
}


def get_base_free_mandelboxes(instance_type: str) -> int:
    """
    Returns the number of containers an instance of this type can hold
    Args:
        instance_type: Which type of instance this is

    Returns:
        How many containers can it hold
    """
    return type_to_number_map[instance_type]


def find_enabled_regions() -> Any:
    """
    Returns a list of regions that are currently active
    """

    return RegionToAmi.query.filter_by(ami_active=True).distinct(RegionToAmi.region_name)


def find_instance(region: str, client_commit_hash: str) -> Union[str, MandelboxAssignError]:
    """
    Given a list of regions, finds (if it can) an instance in that region
    or a neighboring region with space. If it succeeds, returns the instance name.
    Else, returns None
    Args:
        regions: a list of regions sorted by proximity

    Returns: either an instance name or MandelboxAssignError

    """
    bundled_regions = bundled_region.get(region, []) + [region]
    # InstancesWithRoomForMandelboxes is sorted in DESC
    # with number of mandelboxes running, So doing a
    # query with limit of 1 returns the instance with max
    # occupancy which can improve resource utilization.
    instance_with_max_mandelboxes: Optional[InstancesWithRoomForMandelboxes] = (
        InstancesWithRoomForMandelboxes.query.filter_by(
            commit_hash=client_commit_hash, location=region, status=MandelboxHostState.ACTIVE
        )
        .limit(1)
        .one_or_none()
    )

    if instance_with_max_mandelboxes is None:
        # If we are unable to find the instance in the required region,
        # let's try to find an instance in nearby AZ
        # that doesn't impact the user experience too much.
        active_instances_in_bundled_regions = InstancesWithRoomForMandelboxes.query.filter(
            InstancesWithRoomForMandelboxes.location.in_(bundled_regions)
        ).filter_by(status=MandelboxHostState.ACTIVE)

        # If there are no active instances in nearby regions,
        # return NO_INSTANCE_AVAILABLE
        if active_instances_in_bundled_regions.limit(1).one_or_none() is None:
            return MandelboxAssignError.NO_INSTANCE_AVAILABLE

        # If there was an active instance but none with the right commit hash,
        # return COMMIT_HASH_MISMATCH
        instances_with_correct_commit_hash = (
            active_instances_in_bundled_regions.filter_by(commit_hash=client_commit_hash)
            .limit(1)
            .one_or_none()
        )

        if instances_with_correct_commit_hash is None:
            return MandelboxAssignError.COMMIT_HASH_MISMATCH

        instance_with_max_mandelboxes = instances_with_correct_commit_hash

    if instance_with_max_mandelboxes is None:
        # We should never reach this line, if so return UNDEFINED
        return MandelboxAssignError.UNDEFINED
    else:
        # 5sec arbitrarily decided as sufficient timeout when using with_for_update
        set_local_lock_timeout(5)
        # We are locking InstanceInfo row to ensure that we are not assigning a user/mandelbox
        # to an instance that might be marked as DRAINING. With the locking, the instance will be
        # marked as DRAINING after the assignment is complete but not during the assignment.
        avail_instance: Optional[InstanceInfo] = (
            InstanceInfo.query.filter_by(instance_name=instance_with_max_mandelboxes.instance_name)
            .with_for_update()
            .one_or_none()
        )
        # The instance that was available earlier might be lost before we try to grab a lock.
        if avail_instance is None or avail_instance.status != MandelboxHostState.ACTIVE:
            return MandelboxAssignError.COULD_NOT_LOCK_INSTANCE
        else:
            return str(avail_instance.instance_name)


def _get_num_new_instances(region: str, ami_id: str) -> int:
    """
    This function, given a region and an AMI ID, returns the number of new instances
    that we want to have spun up in that region with that AMI.  Returns
     -sys.maxsize for cases of "that AMI is inactive and we want to get rid of
     all instances running it when possible", since in that case
     we want to remove every instance of that type (and -sys.maxsize functions as
     negative infinity).

     At the moment, our scaling algorithm is
     - 'if we have fewer than DESIRED_FREE_MANDELBOXES mandelboxes in a valid AMI/region pair,
     make DEFAULT_INSTANCE_BUFFER new instances'
     - 'Else, if we have a full instance of extra space more than DESIRED_FREE_MANDELBOXES, try to
     stop an instance'
     - 'Else, we're fine'.
    Args:
        region: which region we care about
        ami_id: which AMI ID we're checking

    Returns: the integer number of new instances we want live

    """
    default_increment = int(current_app.config["DEFAULT_INSTANCE_BUFFER"])
    default_decrement = -1

    # If the region is invalid or the AMI is not current, we want no buffer
    if region not in {x.region_name for x in RegionToAmi.query.all()}:
        whist_logger.info(
            f"Returning negative infinity for (region: {region}, ami_id: {ami_id}) because the"
            f" region {region} is invalid."
        )
        return -maxsize
    active_ami_for_given_region = RegionToAmi.query.filter_by(
        region_name=region, ami_active=True
    ).one_or_none()
    if active_ami_for_given_region is None:
        whist_logger.info(
            f"Returning negative infinity for (region: {region}, ami_id: {ami_id}) because the"
            f" active AMI for region {region} is None."
        )
        return -maxsize
    if ami_id != active_ami_for_given_region.ami_id:
        whist_logger.info(
            f"Returning negative infinity for (region: {region}, ami_id: {ami_id}) because the"
            " provided AMI does not match the expected active AMI for region"
            f" {region} ({active_ami_for_given_region.ami_id})."
        )
        return -maxsize

    # Now, we want to get the average number of mandelboxes per instance in that region
    # and the number of free mandelboxes
    all_instances = list(InstanceInfo.query.filter_by(location=region, aws_ami_id=ami_id).all())
    if len(all_instances) == 0:
        # If there are no instances running, we want one.
        whist_logger.info(
            f"Returning {default_increment} for (region: {region}, ami_id: {ami_id}) because there"
            " are no instances running."
        )
        return default_increment

    all_free_instances = list(
        InstancesWithRoomForMandelboxes.query.filter_by(location=region, aws_ami_id=ami_id).all()
    )
    num_free_mandelboxes = sum(
        instance.mandelbox_capacity - instance.num_running_mandelboxes
        for instance in all_free_instances
    )
    avg_mandelbox_capacity = sum(instance.mandelbox_capacity for instance in all_instances) / len(
        all_instances
    )

    # And then figure out whether to call for an increase (by default_increment)
    # or decrease (by default_decrement).
    # Check config DB for the correct value for the number of desired free mandelboxes.
    desired_free_mandelboxes = int(current_app.config["DESIRED_FREE_MANDELBOXES"])

    if num_free_mandelboxes < desired_free_mandelboxes:
        whist_logger.info(
            f"Returning {default_increment} for (region: {region}, ami_id: {ami_id}) because there"
            f" are only {num_free_mandelboxes} free mandelboxes, but we desire"
            f" {desired_free_mandelboxes}."
        )
        return default_increment

    if num_free_mandelboxes >= (desired_free_mandelboxes + avg_mandelbox_capacity):
        whist_logger.info(
            f"Returning {default_decrement} for (region: {region}, ami_id: {ami_id}) because there"
            f" are {num_free_mandelboxes} free mandelboxes, but we desire"
            f" {desired_free_mandelboxes} and have current avg_mandelbox_capacity"
            f" {avg_mandelbox_capacity}."
        )
        return default_decrement

    whist_logger.info(
        f"Returning 0 for (region: {region}, ami_id: {ami_id}) because no other conditions were"
        f" satisfied. Some more details: num_free_mandelboxes: {num_free_mandelboxes},"
        f" desired_free_mandelboxes: {desired_free_mandelboxes}, avg_mandelbox_capacity:"
        f" {avg_mandelbox_capacity}."
    )
    return 0


# We make a collection of mutexes, one for each region and AMI.
# Defaultdict is a lazy dict, that creates default objects (in this case locks)
# as needed.
# Note that this only works if we have 1 web process,
# Multiprocess support will require DB synchronization/locking
scale_mutex: DefaultDict[str, threading.Lock] = defaultdict(threading.Lock)


def do_scale_up_if_necessary(
    region: str, ami: str, force_buffer: int = 0, **kwargs: Any
) -> List[str]:
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

    Returns: List of names of the instances created, if any.

    """
    new_instance_names = []
    with scale_mutex[f"{region}-{ami}"], kwargs["flask_app"].app_context():

        # num_new indicates how many new instances need to be spun up from
        # the ami that is passed in. Usually, we calculate that through
        # invoking `_get_num_new_instances` function. However, we can chose to
        # ignore that recommendation by passing in the `force_buffer` value.
        num_new = 0
        if force_buffer is not None and force_buffer > 0:
            num_new = force_buffer
        else:
            num_new = _get_num_new_instances(region, ami)

        ami_obj = RegionToAmi.query.filter_by(region_name=region, ami_id=ami).one_or_none()

        if ami_obj:
            whist_logger.info(f"Scaling up {str(num_new)} instances in {region} with AMI ID {ami}")
        else:
            whist_logger.info(
                f"Wanted to scale up {str(num_new)} instances in "
                f"{region} with AMI ID {ami} but no active entry found in database"
            )

        aws_instance_type = current_app.config["AWS_INSTANCE_TYPE_TO_LAUNCH"]
        if num_new > 0:
            client = EC2Client(region_name=region)
            base_name = generate_name(starter_name=f"ec2-{region}")
            base_number_free_mandelboxes = get_base_free_mandelboxes(aws_instance_type)

            num_attempts = 0
            instance_indexes = list(range(num_new))

            # Attempt to start new instances if necessary
            while num_attempts <= MAX_RETRY_ATTEMPTS and len(instance_indexes) > 0:
                whist_logger.info(f"Attempt #{num_attempts} at starting instances")

                # Wait before attempting to start instances again
                if num_attempts >= 1:
                    time.sleep(WAIT_TIME_BEFORE_RETRY_IN_SECONDS)

                num_attempts += 1
                instances_to_retry = []

                for index in instance_indexes:
                    try:
                        instance_ids = client.start_instances(
                            image_id=ami,
                            instance_name=base_name + f"-{index}",
                            num_instances=1,
                            instance_type=aws_instance_type,
                        )
                    except ClientError as error:
                        if error.response["Error"]["Code"] == "InsufficientInstanceCapacity":
                            # This error occurs when AWS is out of EC2 instances of the specific
                            # instance type AWS_INSTANCE_TYPE_TO_LAUNCH, which is out of our control
                            # and should not raise an error.
                            whist_logger.warning(
                                "skipping start instance for instance with "
                                f"image_id: {ami}, "
                                f"instance_name: {base_name}-{index}, "
                                f"num_instances: {1}, "
                                f"instance_type: {aws_instance_type}, "
                                f"error: {error.response['Error']['Code']}"
                            )
                            # We want to attempt to start instance again
                            instances_to_retry.append(index)
                        else:
                            # Any other error may suggest issues within the codebase
                            raise error
                        # Skip adding instance id to list of new instances started
                        continue
                    # Setting last update time to -1 indicates that the instance
                    # hasn't told the webserver it's live yet. We add the rows to
                    # the DB now so that future scaling operations don't
                    # double-scale.
                    new_instance = InstanceInfo(
                        location=region,
                        aws_ami_id=ami,
                        cloud_provider_id=f"aws-{instance_ids[0]}",
                        instance_name=base_name + f"-{index}",
                        aws_instance_type=aws_instance_type,
                        mandelbox_capacity=base_number_free_mandelboxes,
                        last_updated_utc_unix_ms=-1,
                        creation_time_utc_unix_ms=int(time.time() * 1000),
                        status=MandelboxHostState.PRE_CONNECTION,
                        commit_hash=ami_obj.client_commit_hash,
                        ip="",  # Will be set by `host_service` once it boots up.
                    )
                    new_instance_names.append(new_instance.instance_name)
                    db.session.add(new_instance)
                    db.session.commit()

                    whist_logger.info(f"Successfully spun up instance {new_instance.instance_name}")

                # Update the list of instance_indexes to onces that need to be retried
                instance_indexes = instances_to_retry

    return new_instance_names
