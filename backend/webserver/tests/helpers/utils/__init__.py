from random import sample
from typing import Any, List, Optional
from app.models import db, Image
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
    return Image.query.all()


def get_allowed_region_names() -> List[str]:
    """
    Returns a randomly picked region name from RegionToAmi table.
    Args:
        count: Number of random region names to return
    Returns:
            Specified number of random region names or None
    """
    return find_enabled_regions()


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
