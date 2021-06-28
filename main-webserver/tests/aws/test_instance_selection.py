import pytest
from app.helpers.blueprint_helpers.aws.aws_instance_post import find_instance, bundled_region
from tests.constants import CLIENT_COMMIT_HASH_FOR_TESTING


def test_empty_instances():
    """
    Confirms that we don't find any instances on a fresh db
    """
    assert find_instance("us-east-1", CLIENT_COMMIT_HASH_FOR_TESTING) is None


def test_find_initial_instance(bulk_instance):
    """
    Confirms that we find an empty instance
    """
    instance = bulk_instance(location="us-east-1")
    assert find_instance("us-east-1", CLIENT_COMMIT_HASH_FOR_TESTING) == instance.instance_name


def test_find_part_full_instance(bulk_instance):
    """
    Confirms that we find an in-use instance
    """
    instance = bulk_instance(location="us-east-1", associated_containers=3)
    assert find_instance("us-east-1", CLIENT_COMMIT_HASH_FOR_TESTING) == instance.instance_name


def test_find_part_full_instance_order(bulk_instance):
    """
    Confirms that we find an in-use instance
    """
    instance = bulk_instance(location="us-east-1", associated_containers=3)
    _ = bulk_instance(location="us-east-1", associated_containers=2)
    assert find_instance("us-east-1", CLIENT_COMMIT_HASH_FOR_TESTING) == instance.instance_name


def test_no_find_full_instance(bulk_instance):
    """
    Confirms that we don't find a full instance
    """
    _ = bulk_instance(location="us-east-1", associated_containers=10)
    assert find_instance("us-east-1", CLIENT_COMMIT_HASH_FOR_TESTING) is None


def test_no_find_full_small_instance(bulk_instance):
    """
    Confirms that we don't find a full instance with <10 max
    """
    _ = bulk_instance(location="us-east-1", container_capacity=5, associated_containers=5)
    assert find_instance("us-east-1", CLIENT_COMMIT_HASH_FOR_TESTING) is None


@pytest.mark.parametrize(
    "location",
    bundled_region.keys(),
)
def test_assignment_logic(bulk_instance, location):
    # ensures that replacement will work for all regions
    """
    tests 4 properties of our find unassigned algorithm:
    1) we don't return an overfull instance in the requested region
    2) we don't return an overfull instance in a replacement region
    3) we return an available instance in a replacement region
    4) if available, we use the requested region rather than a replacement one
    these properties are tested in order
    """
    replacement_region = bundled_region[location][0]
    bulk_instance(associated_containers=10, location=location)
    assert (
        find_instance(location, CLIENT_COMMIT_HASH_FOR_TESTING) is None
    ), f"we assigned an already full instance in the main region, {location}"
    bulk_instance(associated_containers=10, location=replacement_region)
    assert (
        find_instance(location, CLIENT_COMMIT_HASH_FOR_TESTING) is None
    ), f"we assigned an already full instance in the secondary region, {replacement_region}"
    bulk_instance(location=replacement_region, instance_name="replacement-container")
    assert find_instance(location, CLIENT_COMMIT_HASH_FOR_TESTING) is not None, (
        f"we failed to find the available instance "
        f"in the replacement region {replacement_region}"
    )
    bulk_instance(location=location, instance_name="main-container")
    instance = find_instance(location, CLIENT_COMMIT_HASH_FOR_TESTING)
    assert (
        instance is not None and instance == "main-container"
    ), f"we failed to find the available instance in the main region {location}"
