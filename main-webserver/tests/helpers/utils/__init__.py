from random import sample
import re
from app.models import RegionToAmi


def get_random_regions(count=2):
    all_regions = RegionToAmi.query.filter().all()
    if len(all_regions) >= count:
        randomly_picked_regions = sample(all_regions, k=count)
        return randomly_picked_regions
    else:
        return None


def get_random_region_names(count=2):
    """
    Returns a randomly picked region name from RegionToAmi table.
    Args:
    """
    randomly_picked_regions = get_random_regions(count)
    if len(randomly_picked_regions) >= count:
        return [region.region_name for region in randomly_picked_regions]
    else:
        return None


def get_random_region_name():
    random_region = get_random_region_names(1)
    print(random_region)
    if random_region is not None:
        return random_region[0]
    else:
        return None
