from random import randint
from sys import maxsize
from typing import Any, Callable, Dict, List, Tuple

from flask import Flask

import app.helpers.aws.aws_instance_post as aws_funcs
from app.models import Image, Instance, MandelboxHostState

from tests.helpers.utils import get_allowed_regions


def test_scale_up_single(
    app: Flask,
    hijack_ec2_calls: List[Dict[str, Any]],
    mock_get_num_new_instances: Callable[[Any], None],
    hijack_db: List[Dict[str, Any]],  # pylint: disable=unused-argument
    region_name: str,
) -> None:
    """
    Tests that we successfully scale up a single instance when required.
    Mocks every side-effecting function.
    """
    call_list = hijack_ec2_calls
    mock_get_num_new_instances(1)
    random_region_image_obj = Image.query.filter_by(region=region_name).first()
    aws_funcs.do_scale_up_if_necessary(region_name, random_region_image_obj.image_id, flask_app=app)
    assert len(call_list) == 1
    assert call_list[0]["kwargs"]["image_id"] == random_region_image_obj.image_id


def test_scale_up_multiple(
    app: Flask,
    hijack_ec2_calls: List[Dict[str, Any]],
    mock_get_num_new_instances: Callable[[Any], None],
    hijack_db: List[Dict[str, Any]],  # pylint: disable=unused-argument
    region_name: str,
) -> None:
    """
    Tests that we successfully scale up multiple instances when required.
    Mocks every side-effecting function.
    """
    desired_num = randint(2, 10)
    call_list = hijack_ec2_calls
    mock_get_num_new_instances(desired_num)
    us_east_1_image_obj = Image.query.filter_by(region=region_name).first()
    aws_funcs.do_scale_up_if_necessary(region_name, us_east_1_image_obj.image_id, flask_app=app)
    assert len(call_list) == desired_num
    assert all(elem["kwargs"]["image_id"] == us_east_1_image_obj.image_id for elem in call_list)


def test_buffer_wrong_region() -> None:
    """
    checks that we return -sys.maxsize when we ask about a nonexistent region
    """
    assert (
        aws_funcs._get_num_new_instances(  # pylint: disable=protected-access
            "fake_region", "fake-AMI"
        )
        == -maxsize
    )


def test_buffer_wrong_ami() -> None:
    """
    checks that we return -sys.maxsize when we ask about a nonexistent ami
    """
    assert (
        aws_funcs._get_num_new_instances(  # pylint: disable=protected-access
            "us-east-1", "fake-AMI"
        )
        == -maxsize
    )


def test_buffer_empty(app: Flask, region_ami_pair: Tuple[str, str]) -> None:
    """
    Tests that we ask for a new instance when the buffer is empty
    """
    region_name, ami_id = region_ami_pair
    default_increment = int(app.config["DEFAULT_INSTANCE_BUFFER"])
    assert (
        aws_funcs._get_num_new_instances(region_name, ami_id)  # pylint: disable=protected-access
        == default_increment
    )


def test_buffer_part_full(
    app: Flask, bulk_instance: Callable[..., Instance], region_ami_pair: Tuple[str, str]
) -> None:
    """
    Tests that we ask for a new instance when there's only a full instance running
    """
    region_name, ami_id = region_ami_pair
    bulk_instance(
        aws_ami_id=ami_id, associated_mandelboxes=10, mandelbox_capacity=10, location=region_name
    )
    default_increment = int(app.config["DEFAULT_INSTANCE_BUFFER"])
    assert (
        aws_funcs._get_num_new_instances(region_name, ami_id)  # pylint: disable=protected-access
        == default_increment
    )


def test_buffer_good(
    app: Flask, bulk_instance: Callable[..., Instance], region_ami_pair: Tuple[str, str]
) -> None:
    """
    Tests that we don't ask for a new instance when there's an empty instance running with capacity
    to accomodate our desired buffer capacity for mandelboxes
    """
    region_name, ami_id = region_ami_pair
    desired_free_mandelboxes = int(app.config["DESIRED_FREE_MANDELBOXES"])
    bulk_instance(
        aws_ami_id=ami_id, mandelbox_capacity=desired_free_mandelboxes, location=region_name
    )
    assert (
        aws_funcs._get_num_new_instances(region_name, ami_id)  # pylint: disable=protected-access
        == 0
    )


def test_buffer_with_multiple(
    app: Flask, bulk_instance: Callable[..., Instance], region_ami_pair: Tuple[str, str]
) -> None:
    """
    Tests that we don't ask for a new instance when we have enough space in multiple instances
    """
    region_name, ami_id = region_ami_pair
    desired_free_mandelboxes = int(app.config["DESIRED_FREE_MANDELBOXES"])
    mandelbox_capacity = 10

    first_instance_buffer = int(desired_free_mandelboxes / 2)
    second_instance_buffer = desired_free_mandelboxes - first_instance_buffer

    for instance_buffer in [first_instance_buffer, second_instance_buffer]:
        bulk_instance(
            aws_ami_id=ami_id,
            associated_mandelboxes=mandelbox_capacity - instance_buffer,
            mandelbox_capacity=mandelbox_capacity,
            location=region_name,
        )
    assert (
        aws_funcs._get_num_new_instances(region_name, ami_id)  # pylint: disable=protected-access
        == 0
    )


def test_buffer_with_multiple_draining(
    app: Flask, bulk_instance: Callable[..., Instance], region_ami_pair: Tuple[str, str]
) -> None:
    """
    Tests that we don't ask for a new instance when we have enough space in multiple instances
    and also that draining instances are ignored
    """
    region_name, ami_id = region_ami_pair
    desired_free_mandelboxes = int(app.config["DESIRED_FREE_MANDELBOXES"])
    mandelbox_capacity = 10

    first_instance_buffer = int(desired_free_mandelboxes / 2)
    second_instance_buffer = desired_free_mandelboxes - first_instance_buffer

    for instance_buffer in [first_instance_buffer, second_instance_buffer]:
        bulk_instance(
            aws_ami_id=ami_id,
            associated_mandelboxes=mandelbox_capacity - instance_buffer,
            mandelbox_capacity=mandelbox_capacity,
            location=region_name,
        )

    bulk_instance(
        aws_ami_id=ami_id,
        associated_mandelboxes=0,
        mandelbox_capacity=10,
        status=MandelboxHostState.DRAINING,
        location=region_name,
    )
    bulk_instance(
        aws_ami_id=ami_id,
        associated_mandelboxes=0,
        mandelbox_capacity=10,
        status=MandelboxHostState.DRAINING,
        location=region_name,
    )
    assert (
        aws_funcs._get_num_new_instances(region_name, ami_id)  # pylint: disable=protected-access
        == 0
    )


def test_buffer_overfull(
    app: Flask, bulk_instance: Callable[..., Instance], region_ami_pair: Tuple[str, str]
) -> None:
    """
    Tests that we ask to scale down an instance when we have too much free space
    """
    region_name, ami_id = region_ami_pair
    desired_free_mandelboxes = int(app.config["DESIRED_FREE_MANDELBOXES"])
    mandelbox_capacity = 10
    buffer_threshold = desired_free_mandelboxes + mandelbox_capacity

    buffer_capacity_available = 0
    while True:
        current_instance_buffer = randint(0, mandelbox_capacity)
        bulk_instance(
            aws_ami_id=ami_id,
            associated_mandelboxes=mandelbox_capacity - current_instance_buffer,
            mandelbox_capacity=mandelbox_capacity,
            location=region_name,
        )
        buffer_capacity_available += current_instance_buffer
        # Break out when we allocated more buffer than our threshold.
        if buffer_capacity_available > buffer_threshold:
            break
    assert (
        aws_funcs._get_num_new_instances(region_name, ami_id)  # pylint: disable=protected-access
        == -1
    )


def test_buffer_not_too_full(
    app: Flask, bulk_instance: Callable[..., Instance], region_ami_pair: Tuple[str, str]
) -> None:
    """
    Tests that we don't ask to scale down an instance when we have free space, less than our
    buffer threshold but equal to / more than our desired free space.
    """
    region_name, ami_id = region_ami_pair

    desired_free_mandelboxes = int(app.config["DESIRED_FREE_MANDELBOXES"])
    mandelbox_capacity = 10
    buffer_threshold = desired_free_mandelboxes + mandelbox_capacity

    # Here free space is randomly picked from desired free mandelboxes
    # till our buffer threshold.
    free_space = randint(desired_free_mandelboxes, buffer_threshold - 1)

    first_instance_buffer = int(free_space / 2)
    second_instance_buffer = free_space - first_instance_buffer

    for instance_buffer in [first_instance_buffer, second_instance_buffer]:
        bulk_instance(
            aws_ami_id=ami_id,
            associated_mandelboxes=mandelbox_capacity - instance_buffer,
            mandelbox_capacity=mandelbox_capacity,
            location=region_name,
        )
    assert (
        aws_funcs._get_num_new_instances(region_name, ami_id)  # pylint: disable=protected-access
        == 0
    )


def test_buffer_overfull_split(
    app: Flask, bulk_instance: Callable[..., Instance], region_ami_pair: Tuple[str, str]
) -> None:
    """
    Tests that we ask to scale down an instance when we have too much free space
    over several separate instances i.e buffer capacity for mandelboxes exceeds our
    buffer threshold.
    """
    region_name, ami_id = region_ami_pair

    desired_free_mandelboxes = int(app.config["DESIRED_FREE_MANDELBOXES"])
    mandelbox_capacity = 10
    buffer_threshold = desired_free_mandelboxes + mandelbox_capacity

    buffer_available = 0
    while True:
        current_instance_buffer = randint(0, mandelbox_capacity)
        bulk_instance(
            aws_ami_id=ami_id,
            associated_mandelboxes=mandelbox_capacity - current_instance_buffer,
            mandelbox_capacity=mandelbox_capacity,
            location=region_name,
        )
        buffer_available += current_instance_buffer
        # Break out when we allocated more buffer than our threshold.
        if buffer_available > buffer_threshold:
            break

    assert (
        aws_funcs._get_num_new_instances(region_name, ami_id)  # pylint: disable=protected-access
        == -1
    )


def test_buffer_not_too_full_split(
    app: Flask, bulk_instance: Callable[..., Instance], region_ami_pair: Tuple[str, str]
) -> None:
    """
    Tests that we don't ask to scale down an instance when we have some free space
    over several separate instances that doesn't exceed our buffer threshold which
    equals our desired buffer capacity plus number of mandelboxes each instance can run.
    """
    region_name, ami_id = region_ami_pair

    desired_free_mandelboxes = int(app.config["DESIRED_FREE_MANDELBOXES"])
    mandelbox_capacity = 10
    buffer_threshold = desired_free_mandelboxes + mandelbox_capacity

    buffer_available = 0
    while True:
        current_instance_buffer = randint(0, mandelbox_capacity)
        buffer_available += current_instance_buffer
        if buffer_available < buffer_threshold:
            bulk_instance(
                aws_ami_id=ami_id,
                associated_mandelboxes=mandelbox_capacity - current_instance_buffer,
                mandelbox_capacity=mandelbox_capacity,
                location=region_name,
            )
        else:
            # Here we break out when our buffer capacity is less than buffer threshold but
            # more than/equal to our desired buffer capacity.
            # The second condition is not quite obvious as the first one from the code.
            # Since in each iteration we increase the buffer by `current_instance_buffer`
            # which can range from 0 to mandelbox capacity, so in the last iteration before
            # we cross the buffer_threshold, we should be atleast equal to desired_free_mandelboxes.
            break

    assert (
        aws_funcs._get_num_new_instances(region_name, ami_id)  # pylint: disable=protected-access
        == 0
    )


def test_buffer_region_sensitive(app: Flask, bulk_instance: Callable[..., Instance]) -> None:
    """
    Tests that our buffer is based on region. In this test case, we pick two regions randomly.

    One region will be given more buffer capacity available than buffer_threshold which
    should result in a recommendation from `_get_num_new_instances` for scale down,
    returning a value of -1.

    For the other region, we don't spin up any instance so the recommendation from
    `_get_num_new_instances` should return a configuration value <DEFAULT_INSTANCE_BUFFER>
    """
    randomly_picked_ami_objs = get_allowed_regions()
    assert len(randomly_picked_ami_objs) >= 2
    randomly_picked_ami_objs = randomly_picked_ami_objs[0:2]

    region_ami_pairs = [(ami_obj.region, ami_obj.image_id) for ami_obj in randomly_picked_ami_objs]
    region_ami_with_buffer, region_ami_without_buffer = region_ami_pairs

    desired_free_mandelboxes = int(app.config["DESIRED_FREE_MANDELBOXES"])
    mandelbox_capacity = 10
    buffer_threshold = desired_free_mandelboxes + mandelbox_capacity

    buffer_available = 0
    while True:
        current_instance_buffer = randint(0, mandelbox_capacity)
        bulk_instance(
            aws_ami_id=region_ami_with_buffer[1],
            associated_mandelboxes=mandelbox_capacity - current_instance_buffer,
            mandelbox_capacity=mandelbox_capacity,
            location=region_ami_with_buffer[0],
        )
        buffer_available += current_instance_buffer
        if buffer_available > buffer_threshold:
            # Break out when we allocated more buffer than our threshold.
            break

    assert (
        aws_funcs._get_num_new_instances(  # pylint: disable=protected-access
            region_ami_with_buffer[0], region_ami_with_buffer[1]
        )
        == -1
    )
    default_increment = int(app.config["DEFAULT_INSTANCE_BUFFER"])
    assert (
        aws_funcs._get_num_new_instances(  # pylint: disable=protected-access
            region_ami_without_buffer[0], region_ami_without_buffer[1]
        )
        == default_increment
    )


def test_get_num_mandelboxes() -> None:
    # Ensures our get_num_mandelboxes utility works as expected
    assert aws_funcs.get_base_free_mandelboxes("g4dn.xlarge") == 1
    assert aws_funcs.get_base_free_mandelboxes("g4dn.2xlarge") == 2
    assert aws_funcs.get_base_free_mandelboxes("g4dn.4xlarge") == 3
    assert aws_funcs.get_base_free_mandelboxes("g4dn.8xlarge") == 3
    assert aws_funcs.get_base_free_mandelboxes("g4dn.16xlarge") == 3
    assert aws_funcs.get_base_free_mandelboxes("g4dn.12xlarge") == 12
