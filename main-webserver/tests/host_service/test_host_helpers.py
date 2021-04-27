import pytest
import time


from app.constants.http_codes import BAD_REQUEST, NOT_FOUND, SUCCESS

from app.models import InstanceInfo

from app.helpers.blueprint_helpers.host_service.host_service_post import (
    instance_heartbeat_helper,
    initial_instance_auth_helper,
)


def test_initial_auth_good():
    resp, resp_code = initial_instance_auth_helper("1.1.1.1", "test_instance_id", "us-east-1")
    resp_dict = resp.get_json()
    assert "AuthToken" in resp_dict.keys()
    assert resp_code == SUCCESS
    attempted_inst = InstanceInfo.query.filter_by(instance_id="test_instance_id").one_or_none()
    assert attempted_inst is not None


def test_initial_auth_exists(bulk_instance):
    bulk_instance(instance_name="test_instance_id")
    resp, resp_code = initial_instance_auth_helper("1.1.1.1", "test_instance_id", "us-east-1")
    resp_dict = resp.get_json()
    assert (resp_dict, resp_code) == ({"status": BAD_REQUEST}, BAD_REQUEST)


def test_heartbeat_no_update(bulk_instance):
    bulk_instance(instance_name="test_instance_id", auth_token="test_auth")
    time.sleep(3)
    resp, resp_code = instance_heartbeat_helper(
        "test_auth", "test_instance_id", 1024000, "test", False
    )
    resp_dict = resp.get_json()
    assert (resp_dict, resp_code) == ({"status": SUCCESS}, SUCCESS)
    inst = InstanceInfo.query.get("test_instance_id")
    assert time.time() - inst.last_pinged <= 2


@pytest.mark.skip(reason='Enable when enforce_auth is set to true')
def test_heartbeat_wrong_key(bulk_instance):
    bulk_instance(instance_name="test_instance_id", auth_token="test_auth")
    time.sleep(3)
    resp, resp_code = instance_heartbeat_helper(
        "test_aut", "test_instance_id", 1024000, "test", False
    )
    resp_dict = resp.get_json()
    assert (resp_dict, resp_code) == ({"status": NOT_FOUND}, NOT_FOUND)
    inst = InstanceInfo.query.get("test_instance_id")
    assert time.time() - inst.last_pinged >= 3


def test_heartbeat_no_exist(bulk_instance):
    bulk_instance(instance_name="test_instance_id", auth_token="test_auth")
    time.sleep(3)
    resp, resp_code = instance_heartbeat_helper(
        "test_auth", "test_instance_id2", 1024, "test", False
    )
    resp_dict = resp.get_json()
    assert (resp_dict, resp_code) == ({"status": NOT_FOUND}, NOT_FOUND)
    inst = InstanceInfo.query.get("test_instance_id")
    assert time.time() - inst.last_pinged >= 3


def test_heartbeat_dying(bulk_instance):
    bulk_instance(instance_name="test_instance_id", auth_token="test_auth")
    resp, resp_code = instance_heartbeat_helper(
        "test_auth", "test_instance_id", 1024000, "test", True
    )
    resp_dict = resp.get_json()
    assert (resp_dict, resp_code) == ({"status": SUCCESS}, SUCCESS)
    assert InstanceInfo.query.filter_by(instance_id="test_instance_id").one_or_none() is None


def test_heartbeat_updates(bulk_instance):
    bulk_instance(instance_name="test_instance_id", auth_token="test_auth")
    time.sleep(3)
    resp, resp_code = instance_heartbeat_helper(
        "test_auth", "test_instance_id", 1025000, "test", False
    )
    resp_dict = resp.get_json()
    assert (resp_dict, resp_code) == ({"status": SUCCESS}, SUCCESS)
    inst = InstanceInfo.query.get("test_instance_id")
    assert inst.memoryRemainingInInstance == 1025
    assert time.time() - inst.last_pinged <= 2
