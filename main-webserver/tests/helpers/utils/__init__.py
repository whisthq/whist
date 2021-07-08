from random import sample
from app.models import RegionToAmi


def get_random_regions(count=2):
    """
    Returns a randomly picked region to AMI objects from RegionToAmi table 
    with ami_active flag marked as true
    Args:
        count: Number of random region objects to return
    Returns:
            Specified number of random region objects or None
    """
    all_regions = RegionToAmi.query.filter_by(ami_active=True).all()
    if len(all_regions) >= count:
        randomly_picked_regions = sample(all_regions, k=count)
        return randomly_picked_regions
    else:
        return None


def get_random_region_names(count=2):
    """
    Returns a randomly picked region name from RegionToAmi table.
    Args:
        count: Number of random region names to return
    Returns:
            Specified number of random region names or None
    """
    randomly_picked_regions = get_random_regions(count)
    if len(randomly_picked_regions) >= count:
        return [region.region_name for region in randomly_picked_regions]
    else:
        return None


def get_random_region_name():
    """
        Returns a single randomly picked region name which has an active AMI
        Args:
            None
        Returns:
            A random region name or None
    """
    random_region = get_random_region_names(1)
    if random_region is not None:
        return random_region[0]
    else:
        return None
