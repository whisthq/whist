import pytest
from random import randint
import app.helpers.blueprint_helpers.aws.aws_instance_post as aws_funcs
from app.helpers.utils.aws.base_ec2_client import EC2Client
from app.models import db


@pytest.fixture
def disable_ec2_calls(monkeypatch):
    call_list = []

    def _helper(*args, **kwargs):
        call_list.append({"args": args, "kwargs": kwargs})

    monkeypatch.setattr(EC2Client, "start_instances", _helper)
    monkeypatch.setattr(EC2Client, "stop_instances", _helper)
    yield call_list


@pytest.fixture
def mock_get_buffer(monkeypatch):
    def _patcher(return_value):
        monkeypatch.setattr(
            aws_funcs, "_get_num_new_instances", (lambda *args, **kwargs: return_value)
        )

    yield _patcher


@pytest.fixture
def mock_db(monkeypatch):
    call_list = []

    def _helper(*args, **kwargs):
        call_list.append({"args": args, "kwargs": kwargs})

    def _empty():
        return

    monkeypatch.setattr(db.session, "add", _helper)
    monkeypatch.setattr(db.session, "commit", _empty)
    yield call_list


def test_scale_up_single(disable_ec2_calls, mock_get_buffer, mock_db):
    """
    Tests that we successfully scale up a single instance when required.
    Mocks every side-effecting function.
    """
    call_list = disable_ec2_calls
    mock_get_buffer(1)
    aws_funcs.do_scale_up("us-east-1", "test-AMI")
    assert len(call_list) == 1
    assert call_list[0]["kwargs"]["image_id"] == "test-AMI"


def test_scale_up_multiple(disable_ec2_calls, mock_get_buffer, mock_db):
    """
    Tests that we successfully scale up multiple instances when required.
    Mocks every side-effecting function.
    """
    desired_num = randint(1, 10)
    call_list = disable_ec2_calls
    mock_get_buffer(desired_num)
    aws_funcs.do_scale_up("us-east-1", "test-AMI")
    assert len(call_list) == desired_num
    assert all(elem["kwargs"]["image_id"] == "test-AMI" for elem in call_list)


def test_scale_down_single_available(disable_ec2_calls, mock_get_buffer, bulk_instance):
    """
    Tests that we scale down an instance when desired
    """
    call_list = disable_ec2_calls
    bulk_instance(instance_name="test_instance", ami_id="test-AMI")
    mock_get_buffer(-1)
    aws_funcs.do_scale_down("us-east-1", "test-AMI")
    assert len(call_list) == 1
    assert call_list[0]["args"][1] == ["test_instance"]


def test_scale_down_single_unavailable(disable_ec2_calls, mock_get_buffer, bulk_instance):
    """
    Tests that we don't scale down an instance with running containers
    """
    call_list = disable_ec2_calls
    bulk_instance(instance_name="test_instance", associated_containers=1, ami_id="test-AMI")
    mock_get_buffer(-1)
    aws_funcs.do_scale_down("us-east-1", "test-AMI")
    assert len(call_list) == 0


def test_scale_down_single_wrong_region(disable_ec2_calls, mock_get_buffer, bulk_instance):
    """
    Tests that we don't scale down an instance in a different region
    """
    call_list = disable_ec2_calls
    bulk_instance(
        instance_name="test_instance",
        associated_containers=1,
        ami_id="test-AMI",
        location="us-east-2",
    )
    mock_get_buffer(-1)
    aws_funcs.do_scale_down("us-east-1", "test-AMI")
    assert len(call_list) == 0


def test_scale_down_multiple_available(disable_ec2_calls, mock_get_buffer, bulk_instance):
    pass


def test_scale_down_multiple_partial_available(disable_ec2_calls, mock_get_buffer, bulk_instance):
    pass


def test_buffer_wrong_region(bulk_instance):
    pass


def test_buffer_wrong_ami(bulk_instance):
    pass


def test_buffer_empty(bulk_instance):
    pass


def test_buffer_part_full(bulk_instance):
    pass


def test_buffer_good(bulk_instance):
    pass


def test_buffer_overfull(bulk_instance):
    pass


def test_buffer_region_sensitive(bulk_instance):
    pass
