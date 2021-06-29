from threading import Thread
import time
import uuid
from http import HTTPStatus
from flask import Blueprint, current_app
from flask.json import jsonify
from flask_jwt_extended import jwt_required
from flask_pydantic import validate
from app.validation import MandelboxAssignBody

from app import fractal_pre_process, log_request
from app.constants import CLIENT_COMMIT_HASH_DEV_OVERRIDE
from app.constants.env_names import DEVELOPMENT
from app.helpers.blueprint_helpers.aws.aws_instance_post import do_scale_up_if_necessary
from app.helpers.blueprint_helpers.aws.aws_mandelbox_assign_post import is_user_active
from app.helpers.utils.general.auth import payment_required
from app.helpers.utils.general.limiter import limiter, RATE_LIMIT_PER_MINUTE
from app.helpers.utils.general.logs import fractal_logger
from app.helpers.blueprint_helpers.aws.aws_instance_post import find_instance
from app.models import RegionToAmi, db
from app.models.hardware import MandelboxInfo, InstanceInfo

aws_mandelbox_bp = Blueprint("aws_mandelbox_bp", __name__)


@aws_mandelbox_bp.route("/regions", methods=("GET",))
@log_request
@jwt_required()
def regions():
    """Return the list of regions in which users are allowed to deploy tasks.

    Returns:
        A list of strings, where each string is the name of a region.
    """

    enabled_regions = RegionToAmi.query.filter_by(region_enabled=True).distinct(
        RegionToAmi.region_name
    )

    return jsonify([region.region_name for region in enabled_regions])


@aws_mandelbox_bp.route("/mandelbox/assign", methods=("POST",))
@limiter.limit(RATE_LIMIT_PER_MINUTE)
@fractal_pre_process
@jwt_required()
@payment_required
@validate()
def aws_mandelbox_assign(body: MandelboxAssignBody, **_kwargs):
    if is_user_active(body.username):
        # If the user already has a mandelbox running, don't start up a new one
        return jsonify({"IP": "None"}), HTTPStatus.SERVICE_UNAVAILABLE

    client_commit_hash = None
    if (
        current_app.config["ENVIRONMENT"] == DEVELOPMENT
        and body.client_commit_hash == CLIENT_COMMIT_HASH_DEV_OVERRIDE
    ):
        # This condition is to accomodate the worflow for developers of client_apps
        # to test their changes without needing to update the development database with
        # commit_hashes on their local machines.
        client_commit_hash = (
            RegionToAmi.query.filter_by(region_name=body.region, ami_active=True)
            .one_or_none()
            .client_commit_hash
        )
    else:
        client_commit_hash = body.client_commit_hash

    instance_name = find_instance(body.region, client_commit_hash)
    if instance_name is None:
        fractal_logger.info(f"body.region: {body.region}, body.client_commit_hash: {body.client_commit_hash}")

        if not current_app.testing:
            # If we're not testing, we want to scale up a new instance to handle this load
            # and we know what instance type we're missing from the request
            scaling_thread = Thread(
                target=do_scale_up_if_necessary,
                args=(
                    body.region,
                    RegionToAmi.query.get(
                        {"region_name": body.region, "client_commit_hash": client_commit_hash}
                    ).ami_id,
                ),
            )
            scaling_thread.start()
        return jsonify({"ip": "None", "mandelbox_id": "None"}), HTTPStatus.SERVICE_UNAVAILABLE

    instance = InstanceInfo.query.get(instance_name)
    mandelbox_id = str(uuid.uuid4())
    obj = MandelboxInfo(
        mandelbox_id=mandelbox_id,
        instance_name=instance.instance_name,
        user_id=body.username,
        status="ALLOCATED",
        creation_time_utc_unix_ms=int(time.time()),
    )
    db.session.add(obj)
    db.session.commit()
    if not current_app.testing:
        # If we're not testing, we want to scale new instances in the background.
        # Specifically, we want to scale in the region/AMI pair where we know
        # there's usage -- so we call do_scale_up with the location and AMI of the instance

        scaling_thread = Thread(
            target=do_scale_up_if_necessary, args=(instance.location, instance.ami_id)
        )
        scaling_thread.start()

    return jsonify({"ip": instance.ip, "mandelbox_id": mandelbox_id}), HTTPStatus.ACCEPTED
