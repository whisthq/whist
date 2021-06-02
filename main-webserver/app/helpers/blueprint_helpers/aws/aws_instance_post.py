from typing import Optional
from app.models.hardware import InstanceSorted
from app.helpers.utils.db.db_utils import set_local_lock_timeout

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
