import random
import re
import string
import threading
import time

from collections import defaultdict
from sys import maxsize
from typing import Optional
from flask import current_app
from app.models.hardware import (
    db,
    InstanceSorted,
    RegionToAmi,
    InstanceInfo,
    InstancesWithContainers,
)
from app.helpers.utils.db.db_utils import set_local_lock_timeout
from app.helpers.utils.aws.base_ec2_client import EC2Client

bundled_region = {
    "us-east-1": ["us-east-2"],
    "us-east-2": ["us-east-1"],
    "us-west-1": ["us-west-2"],
    "us-west-2": ["us-west-1"],
}


def generate_name(
    starter_name: str = "",
    test_prefix: bool = False,
    branch: Optional[str] = None,
    commit: Optional[str] = None,
):
    """
    Helper function for generating a name with a random UID
    Args:
        starter_name (Optional[str]): starter string for the name
        test_prefix (Optional[bool]): whether the resource should have a "test-" prefix attached
        branch (Optional[str]): Which branch this instance is being created on
        commit (Optional[str]): Which commit this instance is being created on
    Returns:
        str: the generated name
    """
    branch = branch if branch is not None else current_app.config["APP_GIT_BRANCH"]
    commit = commit if commit is not None else current_app.config["APP_GIT_COMMIT"][0:7]
    # Sanitize branch name to prevent complaints from boto. Note from the
    # python3 docs that if the hyphen is at the beginning or end of the
    # contents of `[]` then it should not be escaped.
    branch = re.sub("[^0-9a-zA-Z-]+", "-", branch, count=0, flags=re.ASCII)

    # Generate our own UID instead of using UUIDs since they make the
    # resource names too long (and therefore tests fail). We would need
    # log_36(16^32) = approx 25 digits of lowercase alphanumerics to get
    # the same entropy as 32 hexadecimal digits (i.e. the output of
    # uuid.uuid4()) but 10 digits gives us more than enough entropy while
    # saving us a lot of characters.
    uid = "".join(random.choice(string.ascii_lowercase + string.digits) for _ in range(10))

    # Note that capacity providers and clusters do not allow special
    # characters like angle brackets, which we were using before to
    # separate the branch name and commit hash. To be safe, we don't use
    # those characters in any of our resource names.
    name = f"{starter_name}-{branch}-{commit}-uid-{uid}"

    test_prefix = test_prefix or current_app.testing
    if test_prefix:
        name = f"test-{name}"

    return name


def find_instance(region: str) -> Optional[str]:
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
        InstanceSorted.query.filter_by(location=region)
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
                InstanceSorted.query.filter_by(location=bundlable_region)
                .limit(1)
                .with_for_update(skip_locked=True)
                .one_or_none()
            )
            if avail_instance is not None:
                break
    if avail_instance is None:
        return avail_instance
    return avail_instance.instance_id


def _get_num_new_instances(region: str, ami_id: str) -> int:
    """
    This function, given a region and an AMI ID, returns the number of new instances
    that we want to have spun up in that region with that AMI.  Returns
     -sys.maxsize for cases of "that AMI is inactive and we want to get rid of
     all instances running it when possible"
    Args:
        region: which region we care about
        ami_id: which AMI ID we're checking

    Returns: the integer number of new instances we want live

    """

    # If the region is invalid or the AMI is not current, we want no buffer
    if region not in {x.region for x in RegionToAmi.query.all()}:
        return -maxsize
    if ami_id != RegionToAmi.query.filter_by(region).get().ami_id:
        return -maxsize
    # Now, we want to get the average number of containers per instance in that region
    # and the number of free containers
    all_instances = list(InstanceInfo.query.filter_by(location=region, ami_id=ami_id).all())

    if len(all_instances) == 0:
        # If there are no instances running, we want one.
        return 1
    all_free_instances = list(
        InstancesWithContainers.query.filter_by(location=region, ami_id=ami_id).all()
    )
    num_free_containers = sum(
        instance.maxContainers - instance.running_containers for instance in all_free_instances
    )
    avg_max_containers = sum(instance.maxContainers for instance in all_instances) / len(
        all_instances
    )

    # And then figure out how many instances we need to spin up/purge to get 10 free total
    desired_free_containers = 10.0

    if num_free_containers < desired_free_containers:
        return 1

    if num_free_containers > (desired_free_containers + avg_max_containers):
        return -1

    return 0


# We make a collection of mutexes, one for each region and AMI
# Defaultdict is a lazy dict, that creates default objects (in this case locks)
# as needed
scale_mutex = defaultdict(threading.Lock)


def do_scale_up(region: str, ami: str) -> None:
    """
    Scales up new instances as needed, given a region and AMI to check
    Args:
        region: which region to check for scaling
        ami: which AMI to scale with

    Returns: None

    """
    with scale_mutex[f"{region}-{ami}"]:
        num_new = _get_num_new_instances(region, ami)
        if num_new > 0:
            client = EC2Client(region_name=region)
            base_name = generate_name(starter_name=region)
            # TODO: test that we actually get 16 containers per instance
            base_number_free_containers = 16
            for index in range(num_new):
                client.start_instances(
                    image_id=ami,
                    instance_name=base_name + f"-{index}",
                    num_instances=1,
                )
                # Setting last_pinged to -1 indicates that the
                # instance hasn't told the webserver it's live yet.
                # We add the rows to the DB now so that future scaling operations
                # don't double-scale.
                new_instance = InstanceInfo(
                    location=region,
                    ami_id=ami,
                    instance_id=base_name + f"-{index}",
                    instance_type="g3.4xlarge",
                    maxContainers=base_number_free_containers,
                    last_pinged=-1,
                    created_at=int(time.time()),
                )
                db.session.add(new_instance)
                db.session.commit()


def do_scale_down(region: str, ami: str) -> None:
    """
    Scales down new instances as needed, given a region and AMI to check
    Args:
        region: which region to check for scaling
        ami: which AMI to scale with

    Returns: None

    """
    with scale_mutex[f"{region}-{ami}"]:
        num_new = _get_num_new_instances(region, ami)
        if num_new < 0:
            free_instances = list(
                InstancesWithContainers.query.filter_by(
                    location=region, ami_id=ami, running_containers=0
                )
                .limit(abs(num_new))
                .all()
            )
            if len(free_instances) == 0:
                return
            client = EC2Client(region_name=region)
            client.stop_instances(list(instance.instance_id for instance in free_instances))
