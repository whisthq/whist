import time

from app.constants.http_codes import BAD_REQUEST, NOT_FOUND, SUCCESS

from app.models import InstanceInfo, db

from app.helpers.blueprint_helpers.host_service.host_service_post import (
    instance_heartbeat_helper,
    initial_instance_auth_helper,
)


def test_initial_auth_good(app):
    """
    Tests whether auth works on an empty db
    """
    resp, resp_code = initial_instance_auth_helper(
        "1.1.1.1", "test_instance_id", "g3.4x_large", "us-east-1", "test"
    )
    resp_dict = resp.get_json()
    assert "AuthToken" in resp_dict.keys()
    assert resp_code == SUCCESS
    attempted_inst = InstanceInfo.query.filter_by(instance_id="test_instance_id").one_or_none()

    assert attempted_inst is not None
    db.session.delete(attempted_inst)
    db.session.commit()


def test_initial_auth_exists(bulk_instance):
    """
    Tests whether auth fails if the instance already exists
    """
    bulk_instance(instance_name="test_instance_id")
    resp, resp_code = initial_instance_auth_helper(
        "1.1.1.1", "test_instance_id", "g3.4x_large", "us-east-1", "test"
    )
    resp_dict = resp.get_json()
    assert (resp_dict, resp_code) == ({"status": BAD_REQUEST}, BAD_REQUEST)


def test_heartbeat_no_update(bulk_instance):
    """
    Tests that a heartbeat with no state changes works
    """
    bulk_instance(instance_name="test_instance_id", auth_token="test_auth")
    time.sleep(3)
    resp, resp_code = instance_heartbeat_helper("test_auth", "test_instance_id", 1024000, False)
    resp_dict = resp.get_json()
    assert (resp_dict, resp_code) == ({"status": SUCCESS}, SUCCESS)
    inst = InstanceInfo.query.get("test_instance_id")
    assert time.time() - inst.last_pinged <= 2


def test_heartbeat_wrong_key(bulk_instance):
    """
    Tests that a heartbeat with the wrong auth token fails
    and that it fails with NOT_FOUND
    """
    bulk_instance(instance_name="test_instance_id", auth_token="test_auth")
    time.sleep(3)
    resp, resp_code = instance_heartbeat_helper("bad_token", "test_instance_id", 1024000, False)
    resp_dict = resp.get_json()
    assert (resp_dict, resp_code) == ({"status": NOT_FOUND}, NOT_FOUND)
    inst = InstanceInfo.query.get("test_instance_id")
    assert inst.last_pinged is None


def test_heartbeat_no_exist(bulk_instance):
    """
    Tests that a heartbeat for a nonexistent instance fails
    and that it fails with NOT_FOUND
    """
    bulk_instance(instance_name="test_instance_id", auth_token="test_auth")
    time.sleep(3)
    resp, resp_code = instance_heartbeat_helper("test_auth", "test_instance_id2", 1024, False)
    resp_dict = resp.get_json()
    assert (resp_dict, resp_code) == ({"status": NOT_FOUND}, NOT_FOUND)
    inst = InstanceInfo.query.get("test_instance_id")
    assert inst.last_pinged is None


def test_heartbeat_dying(bulk_instance):
    """
    Tests that a dying heartbeat actually deletes the instance
    """
    bulk_instance(instance_name="test_instance_id", auth_token="test_auth")
    resp, resp_code = instance_heartbeat_helper("test_auth", "test_instance_id", 1024000, True)
    resp_dict = resp.get_json()
    assert (resp_dict, resp_code) == ({"status": SUCCESS}, SUCCESS)
    assert InstanceInfo.query.filter_by(instance_id="test_instance_id").one_or_none() is None


def test_heartbeat_updates(bulk_instance):
    """
    Tests that heartbeats actually update state
    """
    bulk_instance(instance_name="test_instance_id", auth_token="test_auth")
    time.sleep(3)
    resp, resp_code = instance_heartbeat_helper("test_auth", "test_instance_id", 1025000, False)
    resp_dict = resp.get_json()
    assert (resp_dict, resp_code) == ({"status": SUCCESS}, SUCCESS)
    inst = InstanceInfo.query.get("test_instance_id")
    assert inst.memoryRemainingInInstanceInMb == 1025
    assert time.time() - inst.last_pinged <= 2
