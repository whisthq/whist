from threading import Thread
import time
import uuid
from http import HTTPStatus
from flask import Blueprint, current_app
from flask.json import jsonify
from flask_jwt_extended import get_jwt_identity, jwt_required
from flask_pydantic import validate
from app.validation import MandelboxAssignBody

from app import fractal_pre_process, log_request
from app.constants import CLIENT_COMMIT_HASH_DEV_OVERRIDE
from app.constants.env_names import DEVELOPMENT
from app.helpers.blueprint_helpers.aws.aws_instance_post import do_scale_up_if_necessary
from app.helpers.blueprint_helpers.aws.aws_mandelbox_assign_post import is_user_active
from app.helpers.utils.general.limiter import limiter, RATE_LIMIT_PER_MINUTE
from app.helpers.utils.general.logs import fractal_logger
from app.helpers.utils.metrics.flask_app import app_record_metrics
from app.helpers.blueprint_helpers.aws.aws_instance_post import find_instance
from app.models import db, InstanceInfo, MandelboxInfo, RegionToAmi
from payments import payment_required

aws_mandelbox_bp = Blueprint("aws_mandelbox_bp", __name__)


@aws_mandelbox_bp.route("/regions", methods=("GET",))
@log_request
@jwt_required()
def regions():
    """Return the list of regions in which users are allowed to deploy tasks.

    Returns:
        A list of strings, where each string is the name of a region.
    """

    enabled_regions = RegionToAmi.query.filter_by(ami_active=True).distinct(RegionToAmi.region_name)

    return jsonify([region.region_name for region in enabled_regions])


@aws_mandelbox_bp.route("/mandelbox/assign", methods=("POST",))
@limiter.limit(RATE_LIMIT_PER_MINUTE)
@fractal_pre_process
@jwt_required()
@payment_required
@validate()
def aws_mandelbox_assign(body: MandelboxAssignBody, **_kwargs):
    start_time = time.time() * 1000
    care_about_active = False
    username = get_jwt_identity()
    if care_about_active and is_user_active(username):
        # If the user already has a mandelbox running, don't start up a new one
        fractal_logger.debug(f"Returning 503 to user {username} because they are already active.")
        return jsonify({"ip": "None", "mandelbox_id": "None"}), HTTPStatus.SERVICE_UNAVAILABLE
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

    fractal_logger.debug(
        f"Trying to find instance for region {body.region} and commit hash {client_commit_hash}."
    )
    instance_name = find_instance(body.region, client_commit_hash)
    time_when_instance_found = time.time() * 1000
    # How long did it take to find an instance?
    time_to_find_instance = time_when_instance_found - start_time
    fractal_logger.debug(f"It took {time_to_find_instance} ms to find an instance.")
    if instance_name is None:
        fractal_logger.info(
            f"No instance found with body.region: {body.region},\
             body.client_commit_hash: {client_commit_hash}"
        )

        if not current_app.testing:
            # If we're not testing, we want to scale up a new instance to handle this load
            # and we know what instance type we're missing from the request
            ami = RegionToAmi.query.get(
                {"region_name": body.region, "client_commit_hash": client_commit_hash}
            )
            if ami is None:
                fractal_logger.debug(
                    f"No AMI found for region: {body.region}, commit hash: {client_commit_hash}"
                )
            else:
                scaling_thread = Thread(
                    target=do_scale_up_if_necessary,
                    args=(
                        body.region,
                        ami.ami_id,
                    ),
                    kwargs={
                        "flask_app": current_app._get_current_object()  # pylint: disable=protected-access
                    },
                )
                scaling_thread.start()
        fractal_logger.debug(
            f"Returning 503 to user {username} because we didn't find an instance for them."
        )
        return jsonify({"ip": "None", "mandelbox_id": "None"}), HTTPStatus.SERVICE_UNAVAILABLE

    instance = InstanceInfo.query.get(instance_name)
    mandelbox_id = str(uuid.uuid4())
    obj = MandelboxInfo(
        mandelbox_id=mandelbox_id,
        instance_name=instance.instance_name,
        user_id=username,
        status="ALLOCATED",
        creation_time_utc_unix_ms=int(time.time() * 1000),
    )
    db.session.add(obj)
    db.session.commit()
    time_when_container_created = time.time() * 1000
    # How long did it take to create a new container row?
    time_to_create_row = time_when_container_created - time_when_instance_found
    fractal_logger.debug(f"It took {time_to_create_row} ms to create a container row")
    if not current_app.testing:
        # If we're not testing, we want to scale new instances in the background.
        # Specifically, we want to scale in the region/AMI pair where we know
        # there's usage -- so we call do_scale_up with the location and AMI of the request

        scaling_thread = Thread(
            target=do_scale_up_if_necessary,
            args=(
                body.region,
                RegionToAmi.query.get(
                    {"region_name": body.region, "client_commit_hash": client_commit_hash}
                ).ami_id,
            ),
            kwargs={
                "flask_app": current_app._get_current_object()  # pylint: disable=protected-access
            },
        )
        scaling_thread.start()
    # How long did the request take?
    total_request_time = time.time() * 1000 - start_time
    fractal_logger.debug(f"In total, this request took {total_request_time} ms to fulfill")
    app_record_metrics(
        metrics={
            "web.time_to_find_instance": time_to_find_instance,
            "web.time_to_create_row": time_to_create_row,
            "web.total_request_time": total_request_time,
        },
        extra_dims={"task_name": "assign_mandelbox"},
    )
    return jsonify({"ip": instance.ip, "mandelbox_id": mandelbox_id}), HTTPStatus.ACCEPTED
