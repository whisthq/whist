from random import randint
from sys import maxsize
from time import time

import requests


from flask import current_app
from app.models import db, RegionToAmi, InstanceInfo
import app.helpers.blueprint_helpers.aws.aws_instance_post as aws_funcs

from app.constants.instance_state_values import InstanceState

from tests.helpers.utils import get_random_regions


def test_scale_up_single(app, hijack_ec2_calls, mock_get_num_new_instances, hijack_db, region_name):
    """
    Tests that we successfully scale up a single instance when required.
    Mocks every side-effecting function.
    """
    call_list = hijack_ec2_calls
    mock_get_num_new_instances(1)
    random_region_image_obj = RegionToAmi.query.filter_by(
        region_name=region_name, ami_active=True
    ).one_or_none()
    aws_funcs.do_scale_up_if_necessary(region_name, random_region_image_obj.ami_id, flask_app=app)
    assert len(call_list) == 1
    assert call_list[0]["kwargs"]["image_id"] == random_region_image_obj.ami_id


def test_scale_up_multiple(
    app, hijack_ec2_calls, mock_get_num_new_instances, hijack_db, region_name
):
    """
    Tests that we successfully scale up multiple instances when required.
    Mocks every side-effecting function.
    """
    desired_num = randint(1, 10)
    call_list = hijack_ec2_calls
    mock_get_num_new_instances(desired_num)
    us_east_1_image_obj = RegionToAmi.query.filter_by(
        region_name=region_name, ami_active=True
    ).one_or_none()
    aws_funcs.do_scale_up_if_necessary(region_name, us_east_1_image_obj.ami_id, flask_app=app)
    assert len(call_list) == desired_num
    assert all(elem["kwargs"]["image_id"] == us_east_1_image_obj.ami_id for elem in call_list)


def test_scale_down_single_available(
    monkeypatch, hijack_ec2_calls, mock_get_num_new_instances, bulk_instance, region_name
):
    """
    Tests that we scale down an instance when desired
    tests the correct requests, db, and ec2 calls are made.
    """
    post_list = []

    def _helper(*args, **kwargs):
        nonlocal post_list
        post_list.append({"args": args, "kwargs": kwargs})
        raise requests.exceptions.RequestException()

    monkeypatch.setattr(requests, "post", _helper)
    instance = bulk_instance(
        instance_name="test_instance", aws_ami_id="test-AMI", location=region_name
    )
    assert instance.status != InstanceState.DRAINING.value
    mock_get_num_new_instances(-1)
    aws_funcs.try_scale_down_if_necessary(region_name, "test-AMI")
    assert post_list[0]["args"][0] == f"https://{current_app.config['AUTH0_DOMAIN']}/oauth/token"
    db.session.refresh(instance)
    assert instance.status == InstanceState.HOST_SERVICE_UNRESPONSIVE.value


def test_scale_down_single_unavailable(
    hijack_ec2_calls, mock_get_num_new_instances, bulk_instance, region_name
):
    """
    Tests that we don't scale down an instance with running mandelboxes
    """
    instance = bulk_instance(
        instance_name="test_instance",
        associated_mandelboxes=1,
        aws_ami_id="test-AMI",
        location=region_name,
    )
    mock_get_num_new_instances(-1)
    aws_funcs.try_scale_down_if_necessary(region_name, "test-AMI")
    db.session.refresh(instance)
    assert instance.status == "ACTIVE"


def test_scale_down_single_wrong_region(
    hijack_ec2_calls, mock_get_num_new_instances, bulk_instance, region_names
):
    """
    Tests that we don't scale down an instance in a different region
    """
    region_name_1, region_name_2 = region_names
    instance = bulk_instance(
        instance_name="test_instance",
        associated_mandelboxes=1,
        aws_ami_id="test-AMI",
        location=region_name_1,
    )
    mock_get_num_new_instances(-1)
    aws_funcs.try_scale_down_if_necessary(region_name_2, "test-AMI")
    db.session.refresh(instance)
    assert instance.status == "ACTIVE"


def test_scale_down_single_wrong_ami(
    hijack_ec2_calls, mock_get_num_new_instances, bulk_instance, region_name
):
    """
    Tests that we don't scale down an instance with a different AMI
    """
    instance = bulk_instance(
        instance_name="test_instance",
        associated_mandelboxes=1,
        aws_ami_id="test-AMI",
        location=region_name,
    )
    mock_get_num_new_instances(-1)
    aws_funcs.try_scale_down_if_necessary(region_name, "wrong-AMI")
    db.session.refresh(instance)
    assert instance.status == "ACTIVE"


def test_scale_down_multiple_available(
    hijack_ec2_calls, mock_get_num_new_instances, bulk_instance, region_name
):
    """
    Tests that we scale down multiple instances when desired
    """
    desired_num = randint(1, 10)
    instance_list = []
    for instance in range(desired_num):
        bulk_instance(
            instance_name=f"test_instance_{instance}", aws_ami_id="test-AMI", location=region_name
        )
        instance_list.append(f"test_instance_{instance}")
    mock_get_num_new_instances(-desired_num)
    aws_funcs.try_scale_down_if_necessary(region_name, "test-AMI")
    for instance in instance_list:
        instance_info = InstanceInfo.query.get(instance)
        assert instance_info.status == InstanceState.HOST_SERVICE_UNRESPONSIVE.value


def test_scale_down_multiple_partial_available(
    hijack_ec2_calls, mock_get_num_new_instances, bulk_instance, region_name
):
    """
    Tests that we only scale down inactive instances
    """
    desired_num = randint(2, 10)
    num_inactive = randint(1, desired_num - 1)
    num_active = desired_num - num_inactive
    instance_list = []
    active_list = []
    for instance in range(num_inactive):
        bulk_instance(
            instance_name=f"test_instance_{instance}", aws_ami_id="test-AMI", location=region_name
        )
        instance_list.append(f"test_instance_{instance}")
    for instance in range(num_active):
        bulk_instance(
            instance_name=f"test_active_instance_{instance}",
            aws_ami_id="test-AMI",
            associated_mandelboxes=1,
            location=region_name,
        )
        active_list.append(f"test_active_instance_{instance}")
    mock_get_num_new_instances(-desired_num)
    aws_funcs.try_scale_down_if_necessary(region_name, "test-AMI")
    for instance in instance_list:
        instance_info = InstanceInfo.query.get(instance)
        assert instance_info.status == InstanceState.HOST_SERVICE_UNRESPONSIVE.value
    for instance in active_list:
        instance_info = InstanceInfo.query.get(instance)
        assert instance_info.status == InstanceState.ACTIVE.value


def test_lingering_instances(monkeypatch, bulk_instance, region_name):
    """
    Tests that lingering_instances properly drains only those instances that are
    inactive for the specified period of time:
    2 min for running instances, 15 for preconnected instances

    """
    call_set = set()

    def _helper(instance):
        call_set.add(instance.instance_name)

    monkeypatch.setattr(aws_funcs, "drain_instance", _helper)
    bulk_instance(
        instance_name=f"active_instance",
        aws_ami_id="test-AMI",
        location=region_name,
        last_updated_utc_unix_ms=time() * 1000,
    )
    instance_bad_normal = bulk_instance(
        instance_name=f"inactive_instance",
        aws_ami_id="test-AMI",
        location=region_name,
        last_updated_utc_unix_ms=((time() - 121) * 1000),
    )
    instance_bad_preconnect = bulk_instance(
        instance_name=f"inactive_starting_instance",
        aws_ami_id="test-AMI",
        location=region_name,
        status=InstanceState.PRE_CONNECTION.value,
        last_updated_utc_unix_ms=((time() - 1801) * 1000),
    )
    bulk_instance(
        instance_name=f"active_starting_instance",
        aws_ami_id="test-AMI",
        location=region_name,
        status=InstanceState.PRE_CONNECTION.value,
        last_updated_utc_unix_ms=((time() - 121) * 1000),
    )
    aws_funcs.check_and_handle_lingering_instances()
    assert call_set == {instance_bad_normal.instance_name, instance_bad_preconnect.instance_name}


def test_buffer_wrong_region():
    """
    checks that we return -sys.maxsize when we ask about a nonexistent region
    """
    assert aws_funcs._get_num_new_instances("fake_region", "fake-AMI") == -maxsize


def test_buffer_wrong_ami():
    """
    checks that we return -sys.maxsize when we ask about a nonexistent ami
    """
    assert aws_funcs._get_num_new_instances("us-east-1", "fake-AMI") == -maxsize


def test_buffer_empty(region_ami_pair):
    """
    Tests that we ask for a new instance when the buffer is empty
    """
    region_name, ami_id = region_ami_pair
    assert aws_funcs._get_num_new_instances(region_name, ami_id) == 1


def test_buffer_part_full(bulk_instance, region_ami_pair):
    """
    Tests that we ask for a new instance when there's only a full instance running
    """
    region_name, ami_id = region_ami_pair
    bulk_instance(
        aws_ami_id=ami_id, associated_mandelboxes=10, mandelbox_capacity=10, location=region_name
    )
    assert aws_funcs._get_num_new_instances(region_name, ami_id) == 1


def test_buffer_good(app, bulk_instance, region_ami_pair):
    """
    Tests that we don't ask for a new instance when there's an empty instance running with capacity
    to accomodate our desired buffer capacity for mandelboxes
    """
    region_name, ami_id = region_ami_pair
    desired_free_mandelboxes = app.config["DESIRED_FREE_MANDELBOXES"]
    bulk_instance(
        aws_ami_id=ami_id, mandelbox_capacity=desired_free_mandelboxes, location=region_name
    )
    assert aws_funcs._get_num_new_instances(region_name, ami_id) == 0


def test_buffer_with_multiple(app, bulk_instance, region_ami_pair):
    """
    Tests that we don't ask for a new instance when we have enough space in multiple instances
    """
    region_name, ami_id = region_ami_pair
    desired_free_mandelboxes = app.config["DESIRED_FREE_MANDELBOXES"]
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
    assert aws_funcs._get_num_new_instances(region_name, ami_id) == 0


def test_buffer_with_multiple_draining(app, bulk_instance, region_ami_pair):
    """
    Tests that we don't ask for a new instance when we have enough space in multiple instances
    and also that draining instances are ignored
    """
    region_name, ami_id = region_ami_pair
    desired_free_mandelboxes = app.config["DESIRED_FREE_MANDELBOXES"]
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
        status="DRAINING",
        location=region_name,
    )
    bulk_instance(
        aws_ami_id=ami_id,
        associated_mandelboxes=0,
        mandelbox_capacity=10,
        status="DRAINING",
        location=region_name,
    )
    assert aws_funcs._get_num_new_instances(region_name, ami_id) == 0


def test_buffer_overfull(app, bulk_instance, region_ami_pair):
    """
    Tests that we ask to scale down an instance when we have too much free space
    """
    region_name, ami_id = region_ami_pair
    desired_free_mandelboxes = app.config["DESIRED_FREE_MANDELBOXES"]
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
    assert aws_funcs._get_num_new_instances(region_name, ami_id) == -1


def test_buffer_not_too_full(app, bulk_instance, region_ami_pair):
    """
    Tests that we don't ask to scale down an instance when we have free space, less than our
    buffer threshold but equal to / more than our desired free space.
    """
    region_name, ami_id = region_ami_pair

    desired_free_mandelboxes = app.config["DESIRED_FREE_MANDELBOXES"]
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
    assert aws_funcs._get_num_new_instances(region_name, ami_id) == 0


def test_buffer_overfull_split(app, bulk_instance, region_ami_pair):
    """
    Tests that we ask to scale down an instance when we have too much free space
    over several separate instances i.e buffer capacity for mandelboxes exceeds our
    buffer threshold.
    """
    region_name, ami_id = region_ami_pair

    desired_free_mandelboxes = app.config["DESIRED_FREE_MANDELBOXES"]
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

    assert aws_funcs._get_num_new_instances(region_name, ami_id) == -1


def test_buffer_not_too_full_split(app, bulk_instance, region_ami_pair):
    """
    Tests that we don't ask to scale down an instance when we have some free space
    over several separate instances that doesn't exceed our buffer threshold which
    equals our desired buffer capacity plus number of mandelboxes each instance can run.
    """
    region_name, ami_id = region_ami_pair

    desired_free_mandelboxes = app.config["DESIRED_FREE_MANDELBOXES"]
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

    assert aws_funcs._get_num_new_instances(region_name, ami_id) == 0


def test_buffer_region_sensitive(app, bulk_instance):
    """
    Tests that our buffer is based on region. In this test case, we pick two regions randomly.

    One region will be given more buffer capacity available than buffer_threshold which should result in a recommendation
    from `_get_num_new_instances` for scale down, returning a value of -1.

    For the other region, we don't spin up any instance so the recommendation from `_get_num_new_instances` should return
    a configuration value <DEFAULT_INSTANCE_BUFFER>
    """
    randomly_picked_ami_objs = get_random_regions(2)
    assert len(randomly_picked_ami_objs) == 2
    region_ami_pairs = [
        (ami_obj.region_name, ami_obj.ami_id) for ami_obj in randomly_picked_ami_objs
    ]
    region_ami_with_buffer, region_ami_without_buffer = region_ami_pairs

    desired_free_mandelboxes = app.config["DESIRED_FREE_MANDELBOXES"]
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
        aws_funcs._get_num_new_instances(region_ami_with_buffer[0], region_ami_with_buffer[1]) == -1
    )
    assert (
        aws_funcs._get_num_new_instances(region_ami_without_buffer[0], region_ami_without_buffer[1])
        == app.config["DEFAULT_INSTANCE_BUFFER"]
    )


def test_scale_down_harness(monkeypatch, bulk_instance):
    """
    tests that try_scale_down_if_necessary_all_regions actually runs on every region/AMI pair in our db
    """
    call_list = []

    def _helper(*args, **kwargs):
        nonlocal call_list
        call_list.append({"args": args, "kwargs": kwargs})

    monkeypatch.setattr(aws_funcs, "try_scale_down_if_necessary", _helper)
    region_ami_pairs_length = 4
    randomly_picked_ami_objs = get_random_regions(region_ami_pairs_length)
    region_ami_pairs = [
        (ami_obj.region_name, ami_obj.ami_id) for ami_obj in randomly_picked_ami_objs
    ]
    for region, ami_id in region_ami_pairs:
        num_instances = randint(1, 10)
        for _ in range(num_instances):
            bulk_instance(location=region, aws_ami_id=ami_id)

    aws_funcs.try_scale_down_if_necessary_all_regions()
    assert len(call_list) == region_ami_pairs_length
    args = [called["args"] for called in call_list]
    assert set(args) == set(region_ami_pairs)


def test_get_num_mandelboxes():
    # Ensures our get_num_mandelboxes utility works as expected
    assert aws_funcs.get_base_free_mandelboxes("g4dn.xlarge") == 1
    assert aws_funcs.get_base_free_mandelboxes("g4dn.2xlarge") == 1
    assert aws_funcs.get_base_free_mandelboxes("g4dn.4xlarge") == 1
    assert aws_funcs.get_base_free_mandelboxes("g4dn.8xlarge") == 1
    assert aws_funcs.get_base_free_mandelboxes("g4dn.16xlarge") == 1
    assert aws_funcs.get_base_free_mandelboxes("g4dn.12xlarge") == 4
