from random import randint
from sys import maxsize

import requests

from flask import current_app
from app.models import db, RegionToAmi
import app.helpers.blueprint_helpers.aws.aws_instance_post as aws_funcs


def test_scale_up_single(app, hijack_ec2_calls, mock_get_num_new_instances, hijack_db):
    """
    Tests that we successfully scale up a single instance when required.
    Mocks every side-effecting function.
    """
    call_list = hijack_ec2_calls
    mock_get_num_new_instances(1)
    us_east_1_image_obj = RegionToAmi.query.filter_by(
        region_name="us-east-1", ami_active=True
    ).one_or_none()
    aws_funcs.do_scale_up_if_necessary("us-east-1", us_east_1_image_obj.ami_id, flask_app=app)
    assert len(call_list) == 1
    assert call_list[0]["kwargs"]["image_id"] == us_east_1_image_obj.ami_id


def test_scale_up_multiple(app, hijack_ec2_calls, mock_get_num_new_instances, hijack_db):
    """
    Tests that we successfully scale up multiple instances when required.
    Mocks every side-effecting function.
    """
    desired_num = randint(1, 10)
    call_list = hijack_ec2_calls
    mock_get_num_new_instances(desired_num)
    us_east_1_image_obj = RegionToAmi.query.filter_by(
        region_name="us-east-1", ami_active=True
    ).one_or_none()
    aws_funcs.do_scale_up_if_necessary("us-east-1", us_east_1_image_obj.ami_id, flask_app=app)
    assert len(call_list) == desired_num
    assert all(elem["kwargs"]["image_id"] == us_east_1_image_obj.ami_id for elem in call_list)


def test_scale_down_single_available(
    monkeypatch, hijack_ec2_calls, mock_get_num_new_instances, bulk_instance
):
    """
    Tests that we scale down an instance when desired
    tests the correct requests, db, and ec2 calls are made.
    """
    call_list = hijack_ec2_calls
    post_list = []

    def _helper(*args, **kwargs):
        nonlocal post_list
        post_list.append({"args": args, "kwargs": kwargs})
        raise requests.exceptions.RequestException()

    monkeypatch.setattr(requests, "post", _helper)
    instance = bulk_instance(instance_name="test_instance", aws_ami_id="test-AMI")
    assert instance.status != "DRAINING"
    mock_get_num_new_instances(-1)
    aws_funcs.try_scale_down_if_necessary("us-east-1", "test-AMI")
    assert (
        post_list[0]["args"][0]
        == f"http://{instance.ip}:{current_app.config['HOST_SERVICE_PORT']}/drain_and_shutdown"
    )
    db.session.refresh(instance)
    assert instance.status == "DRAINING"
    assert len(call_list) == 1
    assert call_list[0]["args"][1] == ["test_instance"]


def test_scale_down_single_unavailable(hijack_ec2_calls, mock_get_num_new_instances, bulk_instance):
    """
    Tests that we don't scale down an instance with running mandelboxes
    """
    call_list = hijack_ec2_calls
    bulk_instance(instance_name="test_instance", associated_mandelboxes=1, aws_ami_id="test-AMI")
    mock_get_num_new_instances(-1)
    aws_funcs.try_scale_down_if_necessary("us-east-1", "test-AMI")
    assert len(call_list) == 0


def test_scale_down_single_wrong_region(
    hijack_ec2_calls, mock_get_num_new_instances, bulk_instance
):
    """
    Tests that we don't scale down an instance in a different region
    """
    call_list = hijack_ec2_calls
    bulk_instance(
        instance_name="test_instance",
        associated_mandelboxes=1,
        aws_ami_id="test-AMI",
        location="us-east-2",
    )
    mock_get_num_new_instances(-1)
    aws_funcs.try_scale_down_if_necessary("us-east-1", "test-AMI")
    assert len(call_list) == 0


def test_scale_down_single_wrong_ami(hijack_ec2_calls, mock_get_num_new_instances, bulk_instance):
    """
    Tests that we don't scale down an instance with a different AMI
    """
    call_list = hijack_ec2_calls
    bulk_instance(
        instance_name="test_instance",
        associated_mandelboxes=1,
        aws_ami_id="test-AMI",
        location="us-east-2",
    )
    mock_get_num_new_instances(-1)
    aws_funcs.try_scale_down_if_necessary("us-east-1", "wrong-AMI")
    assert len(call_list) == 0


def test_scale_down_multiple_available(hijack_ec2_calls, mock_get_num_new_instances, bulk_instance):
    """
    Tests that we scale down multiple instances when desired
    """
    call_list = hijack_ec2_calls
    desired_num = randint(1, 10)
    instance_list = []
    for instance in range(desired_num):
        bulk_instance(instance_name=f"test_instance_{instance}", aws_ami_id="test-AMI")
        instance_list.append(f"test_instance_{instance}")
    mock_get_num_new_instances(-desired_num)
    aws_funcs.try_scale_down_if_necessary("us-east-1", "test-AMI")
    assert len(call_list) == desired_num
    assert set(item["args"][1][0] for item in call_list) == set(instance_list)


def test_scale_down_multiple_partial_available(
    hijack_ec2_calls, mock_get_num_new_instances, bulk_instance
):
    """
    Tests that we only scale down inactive instances
    """
    call_list = hijack_ec2_calls
    desired_num = randint(2, 10)
    num_inactive = randint(1, desired_num - 1)
    num_active = desired_num - num_inactive
    instance_list = []
    for instance in range(num_inactive):
        bulk_instance(instance_name=f"test_instance_{instance}", aws_ami_id="test-AMI")
        instance_list.append(f"test_instance_{instance}")
    for instance in range(num_active):
        bulk_instance(
            instance_name=f"test_active_instance_{instance}",
            aws_ami_id="test-AMI",
            associated_mandelboxes=1,
        )
    mock_get_num_new_instances(-desired_num)
    aws_funcs.try_scale_down_if_necessary("us-east-1", "test-AMI")
    assert len(call_list) == num_inactive
    assert set(item["args"][1][0] for item in call_list) == set(instance_list)


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


def test_buffer_empty(region_to_ami_map):
    """
    Tests that we ask for a new instance when the buffer is empty
    """
    good_ami = region_to_ami_map["us-east-1"]
    assert aws_funcs._get_num_new_instances("us-east-1", good_ami) == 1


def test_buffer_part_full(region_to_ami_map, bulk_instance):
    """
    Tests that we ask for a new instance when there's only a full instance running
    """
    good_ami = region_to_ami_map["us-east-1"]
    bulk_instance(aws_ami_id=good_ami, associated_mandelboxes=10, mandelbox_capacity=10)
    assert aws_funcs._get_num_new_instances("us-east-1", good_ami) == 1


def test_buffer_good(region_to_ami_map, bulk_instance):
    """
    Tests that we don't ask for a new instance when there's an empty instance running
    """
    good_ami = region_to_ami_map["us-east-1"]
    bulk_instance(aws_ami_id=good_ami, mandelbox_capacity=10)
    assert aws_funcs._get_num_new_instances("us-east-1", good_ami) == 0


def test_buffer_with_multiple(region_to_ami_map, bulk_instance):
    """
    Tests that we don't ask for a new instance when we have enough space in multiple instances
    """
    good_ami = region_to_ami_map["us-east-1"]
    bulk_instance(aws_ami_id=good_ami, associated_mandelboxes=5, mandelbox_capacity=10)
    bulk_instance(aws_ami_id=good_ami, associated_mandelboxes=5, mandelbox_capacity=10)
    assert aws_funcs._get_num_new_instances("us-east-1", good_ami) == 0


def test_buffer_overfull(region_to_ami_map, bulk_instance):
    """
    Tests that we ask to scale down an instance when we have too much free space
    """
    good_ami = region_to_ami_map["us-east-1"]
    bulk_instance(aws_ami_id=good_ami, associated_mandelboxes=0, mandelbox_capacity=10)
    bulk_instance(aws_ami_id=good_ami, associated_mandelboxes=0, mandelbox_capacity=10)
    assert aws_funcs._get_num_new_instances("us-east-1", good_ami) == -1


def test_buffer_not_too_full(region_to_ami_map, bulk_instance):
    """
    Tests that we don't ask to scale down an instance when we have some free space
    """
    good_ami = region_to_ami_map["us-east-1"]
    bulk_instance(aws_ami_id=good_ami, associated_mandelboxes=5, mandelbox_capacity=10)
    bulk_instance(aws_ami_id=good_ami, associated_mandelboxes=0, mandelbox_capacity=10)
    assert aws_funcs._get_num_new_instances("us-east-1", good_ami) == 0


def test_buffer_overfull_split(region_to_ami_map, bulk_instance):
    """
    Tests that we ask to scale down an instance when we have too much free space
    over several separate instances
    """
    good_ami = region_to_ami_map["us-east-1"]
    bulk_instance(aws_ami_id=good_ami, associated_mandelboxes=9, mandelbox_capacity=10)
    bulk_instance(aws_ami_id=good_ami, associated_mandelboxes=1, mandelbox_capacity=10)
    bulk_instance(aws_ami_id=good_ami, associated_mandelboxes=0, mandelbox_capacity=10)
    assert aws_funcs._get_num_new_instances("us-east-1", good_ami) == -1


def test_buffer_not_too_full_split(region_to_ami_map, bulk_instance):
    """
    Tests that we don't ask to scale down an instance when we have some free space
    over several separate instances
    """
    good_ami = region_to_ami_map["us-east-1"]
    bulk_instance(aws_ami_id=good_ami, associated_mandelboxes=9, mandelbox_capacity=10)
    bulk_instance(aws_ami_id=good_ami, associated_mandelboxes=4, mandelbox_capacity=10)
    bulk_instance(aws_ami_id=good_ami, associated_mandelboxes=0, mandelbox_capacity=10)
    assert aws_funcs._get_num_new_instances("us-east-1", good_ami) == 0


def test_buffer_region_sensitive(region_to_ami_map, bulk_instance):
    """
    Tests that our buffer is based on region
    """
    good_ami_1 = region_to_ami_map["us-east-1"]
    good_ami_2 = region_to_ami_map["us-east-2"]
    bulk_instance(aws_ami_id=good_ami_1, associated_mandelboxes=9, mandelbox_capacity=10)
    bulk_instance(aws_ami_id=good_ami_1, associated_mandelboxes=1, mandelbox_capacity=10)
    bulk_instance(aws_ami_id=good_ami_1, associated_mandelboxes=0, mandelbox_capacity=10)
    assert aws_funcs._get_num_new_instances("us-east-1", good_ami_1) == -1
    assert aws_funcs._get_num_new_instances("us-east-2", good_ami_2) == 1


def test_scale_down_harness(monkeypatch, bulk_instance):
    """
    tests that try_scale_down_if_necessary_all_regions actually runs on every region/AMI pair in our db
    """
    call_list = []

    def _helper(*args, **kwargs):
        nonlocal call_list
        call_list.append({"args": args, "kwargs": kwargs})

    monkeypatch.setattr(aws_funcs, "try_scale_down_if_necessary", _helper)
    bulk_instance(location="us-east-1", aws_ami_id="ami-00c40082600650a9a")
    bulk_instance(location="us-east-1", aws_ami_id="ami-00c40082600650a9a")
    bulk_instance(location="us-east-1", aws_ami_id="ami-00c40082600650a9a")
    bulk_instance(location="us-east-1", aws_ami_id="ami-00c40082600650a9b")
    bulk_instance(location="us-east-2", aws_ami_id="ami-0a7da7479f37c924a")
    bulk_instance(location="us-east-2", aws_ami_id="ami-0a7da7479f37c924b")
    aws_funcs.try_scale_down_if_necessary_all_regions()
    assert len(call_list) == 4
    args = [called["args"] for called in call_list]
    assert set(args) == {
        ("us-east-1", "ami-00c40082600650a9a"),
        ("us-east-1", "ami-00c40082600650a9b"),
        ("us-east-2", "ami-0a7da7479f37c924a"),
        ("us-east-2", "ami-0a7da7479f37c924b"),
    }
