from random import sample
from typing import Any, List, Optional
from app.database.models.cloud import InstanceStatusChanges, RegionToAmi, db
from app.helpers.aws.aws_instance_post import find_enabled_regions


def get_allowed_regions() -> Any:
    """
    Returns a randomly picked region to AMI objects from RegionToAmi table
    with ami_active flag marked as true
    Args:
        count: Number of random region objects to return
    Returns:
            Specified number of random region objects or None
    """
    return RegionToAmi.query.filter_by(ami_active=True).all()


def get_allowed_region_names() -> List[str]:
    """
    Returns a randomly picked region name from RegionToAmi table.
    Args:
        count: Number of random region names to return
    Returns:
            Specified number of random region names or None
    """
    allowed_regions = find_enabled_regions()
    return [region.region_name for region in allowed_regions]


def get_random_region_name() -> Optional[str]:
    """
    Returns a single randomly picked region name which has an active AMI
    Args:
        None
    Returns:
        A random region name or None
    """
    regions = get_allowed_region_names()
    if not regions:
        return None
    else:
        return sample(regions, k=1)[0]


def update_status_change_time(timestamp: Any, instance_name: str) -> None:
    """
    Update the most recent instance status change matching the given instance name

    Arguments:
        timestamp: new time to update the timestamp field
        instance_name: name of instance being updated
    """

    instance = InstanceStatusChanges.query.filter(
        InstanceStatusChanges.instance_name == instance_name
    ).first()
    instance.timestamp = timestamp
    db.session.commit()


def set_protected_region_to_ami(region: str, ami: str, commit_hash: str, active: bool) -> None:
    """
    Adds a new entry that is protected from scale down in region to ami

    Arguments:
        region: region name of new entry
        ami: the id of the ami of new entry
        commit_hash: client's commit hash of new entry
    """

    new_region_to_ami = RegionToAmi(
        region_name=region,
        ami_id=ami,
        ami_active=active,
        client_commit_hash=commit_hash,
        protected_from_scale_down=True,
    )
    db.session.add(new_region_to_ami)
    db.session.commit()
