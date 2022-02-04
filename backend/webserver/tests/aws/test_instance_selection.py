from random import randint
from typing import Callable

import pytest

from app.models import Instance, MandelboxHostState
from app.helpers.aws.aws_instance_post import find_instance, bundled_region
from app.constants.mandelbox_assign_error_names import MandelboxAssignError
from tests.constants import CLIENT_COMMIT_HASH_FOR_TESTING, INCORRECT_COMMIT_HASH_FOR_TESTING


def test_empty_instances(region_name: str) -> None:
    """
    Confirms that we don't find any instances on a fresh db
    """
    assert (
        find_instance(region_name, CLIENT_COMMIT_HASH_FOR_TESTING)
        is MandelboxAssignError.NO_INSTANCE_AVAILABLE
    )


def test_find_initial_instance(bulk_instance: Callable[..., Instance], region_name: str) -> None:
    """
    Confirms that we find an empty instance
    """
    instance = bulk_instance(location=region_name)
    assert find_instance(region_name, CLIENT_COMMIT_HASH_FOR_TESTING) == instance.id


def test_find_part_full_instance(bulk_instance: Callable[..., Instance], region_name: str) -> None:
    """
    Confirms that we find an in-use instance
    """
    instance = bulk_instance(location=region_name, associated_mandelboxes=3)
    assert find_instance(region_name, CLIENT_COMMIT_HASH_FOR_TESTING) == instance.id


def test_find_part_full_instance_order(
    bulk_instance: Callable[..., Instance], region_name: str
) -> None:
    """
    Confirms that we find an in-use instance with max occupancy
    """
    max_occupancy = 7
    instance_with_max_occupancy = bulk_instance(
        location=region_name, associated_mandelboxes=max_occupancy
    )
    for _ in range(10):
        # Generating multiple instances with occupancy less than our max_occupied instance
        bulk_instance(location=region_name, associated_mandelboxes=randint(0, max_occupancy - 1))
    assert (
        find_instance(region_name, CLIENT_COMMIT_HASH_FOR_TESTING) == instance_with_max_occupancy.id
    )


def test_no_find_instance_with_incorrect_commit_hash(
    bulk_instance: Callable[..., Instance], region_name: str
) -> None:
    """
    Confirms that we find an empty instance
    """
    bulk_instance(location=region_name)
    assert (
        find_instance(region_name, INCORRECT_COMMIT_HASH_FOR_TESTING)
        is MandelboxAssignError.COMMIT_HASH_MISMATCH
    )


def test_no_find_full_instance(bulk_instance: Callable[..., Instance], region_name: str) -> None:
    """
    Confirms that we don't find a full instance
    """
    _ = bulk_instance(location=region_name, associated_mandelboxes=10)
    assert (
        find_instance(region_name, CLIENT_COMMIT_HASH_FOR_TESTING)
        is MandelboxAssignError.NO_INSTANCE_AVAILABLE
    )


def test_no_find_pre_connected_instance(
    bulk_instance: Callable[..., Instance], region_name: str
) -> None:
    """
    Confirms that we don't find a pre-connection instance
    """
    _ = bulk_instance(
        location=region_name, associated_mandelboxes=0, status=MandelboxHostState.PRE_CONNECTION
    )
    assert (
        find_instance(region_name, CLIENT_COMMIT_HASH_FOR_TESTING)
        is MandelboxAssignError.NO_INSTANCE_AVAILABLE
    )


def test_no_find_full_small_instance(
    bulk_instance: Callable[..., Instance], region_name: str
) -> None:
    """
    Confirms that we don't find a full instance with <10 max
    """
    _ = bulk_instance(location=region_name, mandelbox_capacity=5, associated_mandelboxes=5)
    assert (
        find_instance(region_name, CLIENT_COMMIT_HASH_FOR_TESTING)
        is MandelboxAssignError.NO_INSTANCE_AVAILABLE
    )


@pytest.mark.parametrize(
    "location",
    bundled_region.keys(),
)
def test_assignment_logic(bulk_instance: Callable[..., Instance], location: str) -> None:
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
    bulk_instance(associated_mandelboxes=10, location=location)
    assert (
        find_instance(location, CLIENT_COMMIT_HASH_FOR_TESTING)
        is MandelboxAssignError.NO_INSTANCE_AVAILABLE
    ), f"we assigned an already full instance in the main region, {location}"
    bulk_instance(associated_mandelboxes=10, location=replacement_region)
    assert (
        find_instance(location, CLIENT_COMMIT_HASH_FOR_TESTING)
        is MandelboxAssignError.NO_INSTANCE_AVAILABLE
    ), f"we assigned an already full instance in the secondary region, {replacement_region}"
    bulk_instance(location=replacement_region, instance_name="replacement-mandelbox")
    assert find_instance(location, CLIENT_COMMIT_HASH_FOR_TESTING) is not None, (
        "we failed to find the available instance in the replacement region"
        f" {replacement_region} for original region {location}."
    )
    bulk_instance(location=location, instance_name="main-mandelbox")
    instance_name = find_instance(location, CLIENT_COMMIT_HASH_FOR_TESTING)
    assert (
        instance_name is not None and instance_name == "main-mandelbox"
    ), f"we failed to find the available instance in the main region {location}"
