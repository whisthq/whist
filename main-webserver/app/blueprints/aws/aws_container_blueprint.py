from threading import Thread
import time
import uuid
from flask import Blueprint, current_app
from flask.json import jsonify
from flask_jwt_extended import jwt_required
from flask_pydantic import validate
from app.validation import MandelboxAssignBody

from app import fractal_pre_process, log_request
from app.constants.http_codes import (
    ACCEPTED,
    RESOURCE_UNAVAILABLE,
)
from app.helpers.blueprint_helpers.aws.aws_instance_post import do_scale_up_if_necessary
from app.helpers.blueprint_helpers.aws.aws_container_assign_post import is_user_active
from app.helpers.utils.general.auth import payment_required
from app.helpers.utils.general.limiter import limiter, RATE_LIMIT_PER_MINUTE
from app.helpers.blueprint_helpers.aws.aws_instance_post import find_instance
from app.models import RegionToAmi, db
from app.models.hardware import ContainerInfo, InstanceInfo

aws_container_bp = Blueprint("aws_container_bp", __name__)


@aws_container_bp.route("/regions", methods=("GET",))
@log_request
@jwt_required()
def regions():
    """Return the list of regions in which users are allowed to deploy tasks.

    Returns:
        A list of strings, where each string is the name of a region.
    """

    allowed_regions = RegionToAmi.query.filter_by(allowed=True)

    return jsonify([region.region_name for region in allowed_regions])


@aws_container_bp.route("/mandelbox/assign", methods=("POST",))
@limiter.limit(RATE_LIMIT_PER_MINUTE)
@fractal_pre_process
@jwt_required()
@payment_required
@validate()
def aws_container_assign(body: MandelboxAssignBody, **_kwargs):
    if is_user_active(body.username):
        # If the user already has a container running, don't start up a new one
        return jsonify({"IP": "None"}), RESOURCE_UNAVAILABLE
    instance_id = find_instance(body.region, body.client_commit_hash)
    if instance_id is None:

        if not current_app.testing:
            # If we're not testing, we want to scale up a new instance to handle this load
            # and we know what instance type we're missing from the request
            scaling_thread = Thread(
                target=do_scale_up_if_necessary,
                args=(
                    body.region,
                    RegionToAmi.query.get(body.region, body.client_commit_hash).ami_id,
                ),
            )
            scaling_thread.start()
        return jsonify({"ip": "None", "mandelbox_id": "None"}), RESOURCE_UNAVAILABLE

    instance = InstanceInfo.query.get(instance_id)
    container_id = str(uuid.uuid4())
    obj = ContainerInfo(
        container_id=container_id,
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

    return jsonify({"ip": instance.ip, "mandelbox_id": container_id}), ACCEPTED
