import pytest

import app.helpers.blueprint_helpers.aws.aws_instance_post as aws_funcs
from app.helpers.utils.aws.base_ec2_client import EC2Client
from app.models import db, RegionToAmi


@pytest.fixture
def hijack_ec2_calls(monkeypatch):
    """
    This fixture mocks any calls to EC2client side-effecting endpoints
    and just puts arguments to those calls into a list.
    Use it to confirm that things interfacing with the EC2 client are
    doing so correctly, _without_ risk of side effects.
    """
    call_list = []

    def _helper(*args, **kwargs):
        call_list.append({"args": args, "kwargs": kwargs})
        return ["test_id"]

    monkeypatch.setattr(EC2Client, "start_instances", _helper)
    monkeypatch.setattr(EC2Client, "stop_instances", _helper)
    yield call_list


@pytest.fixture
def mock_get_num_new_instances(monkeypatch):
    """
    This fixture mocks the _get_num_new_instances function
    from aws_instance_post, making it easy to set what that function should return.
    """

    def _patcher(return_value):
        monkeypatch.setattr(
            aws_funcs, "_get_num_new_instances", (lambda *args, **kwargs: return_value)
        )

    yield _patcher


@pytest.fixture
def hijack_db(monkeypatch):
    """
    This fixture mocks any calls to DB side effecting endpoints,
    putting added objects into a list.
    Use it to confirm that things interfacing with the DB in generic ways
    do so correctly, without risk of side effects.
    """
    call_list = []

    def _helper(*args, **kwargs):
        call_list.append({"args": args, "kwargs": kwargs})

    def _empty():
        return

    monkeypatch.setattr(db.session, "add", _helper)
    monkeypatch.setattr(db.session, "commit", _empty)
    yield call_list


@pytest.fixture(autouse=False)
def set_amis_state():
    amis = []
    amis_original_enabled_value = []

    def _setter(amis_to_be_modified, enabled_state):
        for ami in amis_to_be_modified:
            amis_original_enabled_value.append(ami.enabled)
            ami.enabled = enabled_state
        db.session.commit()

    yield _setter
    for ami, original_enabled_value in zip(amis, amis_original_enabled_value):
        ami.enabled = original_enabled_value
    db.session.commit()
