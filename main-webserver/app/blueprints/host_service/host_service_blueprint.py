from flask import Blueprint
from flask.json import jsonify

from app import fractal_pre_process
from app.constants.http_codes import SUCCESS, ACCEPTED, BAD_REQUEST, FORBIDDEN
from app.helpers.blueprint_helpers.host_service.host_service_post import (
    initial_instance_auth_helper,
    instance_heartbeat_helper,
)

host_service_bp = Blueprint("host_service_bp", __name__)


@host_service_bp.route("/host_service/auth", methods=("POST",))
@fractal_pre_process
def host_service_auth(**kwargs):
    body = kwargs.pop("body")
    address = kwargs.pop("received_from")

    try:
        instance_id = body.pop("InstanceID")
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
        auth_token = body.pop("AuthToken")
        instance_id = body.pop("InstanceID")
    except KeyError:
        return jsonify({"status": BAD_REQUEST}), BAD_REQUEST

    # TODO: replace with a helper checking db for instance_id, address, auth_token
    valid_auth = True
    if not valid_auth:
        return jsonify({"status": FORBIDDEN}), FORBIDDEN

    try:
        timestamp = body.pop("Timestamp")
        heartbeat_number = body.pop("HeartbeatNumber")
        instance_type = body.pop("InstanceType")
        total_ram_kb = body.pop("TotalRAMinKB")
        free_ram_kb = body.pop("FreeRAMinKB")
        available_ram_kb = body.pop("AvailRAMinKB")
        dying_heartbeat = body.pop("IsDyingHeartbeat")
    except KeyError:
        return jsonify({"status": BAD_REQUEST}), BAD_REQUEST

    return jsonify({"status": ACCEPTED}), ACCEPTED
