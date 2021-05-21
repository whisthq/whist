from typing import Optional
from app.models.hardware import InstanceSorted

bundled_region = {
    "us-east-1": ["us-east-2"],
    "us-east-2": ["us-east-1"],
    "us-west-1": ["us-west-2"],
    "us-west-2": ["us-west-1"],
}


def find_instance(region: str) -> Optional[str]:
    """
    Given a region, finds (if it can) an instance in that region or a neighboring region with space
    If it succeeds, returns the instance ID.  Else, returns None
    Args:
        region: which region to search

    Returns: either a good instance ID or None

    """
    avail_instance: Optional[InstanceSorted] = (
        InstanceSorted.query.filter_by(location=region)
        .limit(1)
        .with_for_update(skip_locked=True)
        .one_or_none()
    )
    if avail_instance is None:
        # check each replacement region for available containers
        for bundlable_region in bundled_region.get(region, []):
            # 30sec arbitrarily decided as sufficient timeout when using with_for_update
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
