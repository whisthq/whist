from flask import Blueprint
from flask.json import jsonify

from app import fractalPreProcess
from app.constants.http_codes import SUCCESS, ACCEPTED, BAD_REQUEST, FORBIDDEN

import os


host_service_bp = Blueprint("host_service_bp", __name__)


@host_service_bp.route("/host_service/auth", methods=("POST",))
@fractalPreProcess
def host_service_auth(**kwargs):
    body = kwargs.pop("body")
    address = kwargs.pop("received_from")
    try:
        instance_id = body.pop("InstanceID")
    except KeyError:
        return jsonify({"status": BAD_REQUEST}), BAD_REQUEST

    # TODO: replace with a real celery task taking in instance_id and ip address and eventually writing an entry to the db with pkey instance_id and an authentication token
    # TODO: update ecs-host-service to wait for task success via /status/<task id> and then pull the authentication token from the response. Do this in a separate goroutine
    auth_token = os.urandom(16).hex()
    already_happened = False

    if already_happened:
        return jsonify({"status": BAD_REQUEST}), BAD_REQUEST
    return jsonify({"AuthToken": auth_token}), SUCCESS


@host_service_bp.route("/host_service/heartbeat", methods=("POST",))
@fractalPreProcess
def host_service_heartbeat(**kwargs):
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
