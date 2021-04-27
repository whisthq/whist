from json import loads

from app.constants.http_codes import BAD_REQUEST, SUCCESS

from app.models import InstanceInfo

from app.helpers.blueprint_helpers.host_service.host_service_post import (
    instance_heartbeat_helper,
    initial_instance_auth_helper,
)


def test_initial_auth_good():
    resp, resp_code = initial_instance_auth_helper("1.1.1.1", "test_instance_id", "us-east-1")
    resp_dict = loads(resp)
    assert "AuthToken" in resp_dict.keys()
    assert resp_code == SUCCESS
    attempted_inst = InstanceInfo.query.filter_by(instance_id="test_instance_id").one_or_none()
    assert attempted_inst is not None


def test_initial_auth_exists(bulk_instance):
    bulk_instance(instance_name="test_instance_id")
    resp, resp_code = initial_instance_auth_helper("1.1.1.1", "test_instance_id", "us-east-1")
    resp_dict = loads(resp)
    assert (resp_dict, resp_code) == ({"status": BAD_REQUEST}, BAD_REQUEST)


def test_heartbeat_no_update(bulk_instance):
    pass


def test_heartbeat_no_exist(bulk_instance):
    pass


def test_heartbeat_wrong_key(bulk_instance):
    pass


def test_heartbeat_no_exist(bulk_instance):
    pass


def test_heartbeat_dying(bulk_instance):
    pass


def test_heartbeat_updates(bulk_instance):
    pass
