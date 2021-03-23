from flask import Blueprint
from flask.json import jsonify

from app import fractal_pre_process
from app.constants.http_codes import BAD_REQUEST
from app.helpers.blueprint_helpers.host_service.host_service_post import (
    initial_instance_auth_helper,
    instance_heartbeat_helper,
)

host_service_bp = Blueprint("host_service_bp", __name__)


@host_service_bp.route("/host_service/auth", methods=("POST",))
@fractal_pre_process
def host_service_auth(**kwargs):
    body = kwargs.pop("body")
    address: str = kwargs.pop("received_from")

    try:
        instance_id: str = body.pop("InstanceID")
    except KeyError:
        return jsonify({"status": BAD_REQUEST}), BAD_REQUEST

    return initial_instance_auth_helper(address, instance_id)


@host_service_bp.route("/host_service/heartbeat", methods=("POST",))
@fractal_pre_process
def host_service_heartbeat(**kwargs):
    # pylint: disable=unused-variable
    body = kwargs.pop("body")
    address = kwargs.pop("received_from")

    try:
        auth_token: str = body.pop("AuthToken")
        instance_id: str = body.pop("InstanceID")
    except KeyError:
        return jsonify({"status": BAD_REQUEST}), BAD_REQUEST

    try:
        timestamp = body.pop("Timestamp")
        heartbeat_number = body.pop("HeartbeatNumber")
        instance_type: str = body.pop("InstanceType")
        total_ram_kb = body.pop("TotalRAMinKB")
        free_ram_kb: int = body.pop("FreeRAMinKB")
        available_ram_kb = body.pop("AvailRAMinKB")
        dying_heartbeat = body.pop("IsDyingHeartbeat")
    except KeyError:
        return jsonify({"status": BAD_REQUEST}), BAD_REQUEST

    return instance_heartbeat_helper(auth_token, instance_id, free_ram_kb, instance_type)
