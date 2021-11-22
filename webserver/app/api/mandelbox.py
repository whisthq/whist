from threading import Thread
import time
import uuid
from http import HTTPStatus
from flask import Blueprint, current_app, Response
from flask.json import jsonify
from flask_jwt_extended import get_jwt_identity, jwt_required
from flask_pydantic import validate
from typing import Any, Tuple
from app.utils.mandelbox.validation import MandelboxAssignBody

from app import whist_pre_process
from app.constants import CLIENT_COMMIT_HASH_DEV_OVERRIDE
from app.constants.env_names import DEVELOPMENT, LOCAL
from app.helpers.aws.aws_instance_post import do_scale_up_if_necessary
from app.helpers.aws.aws_mandelbox_assign_post import is_user_active
from app.utils.general.limiter import limiter, RATE_LIMIT_PER_MINUTE
from app.utils.general.logs import whist_logger
from app.helpers.general.sanitize import sanitize_email
from app.utils.metrics.flask_app import app_record_metrics
from app.helpers.aws.aws_instance_post import find_instance, find_enabled_regions
from app.database.models.cloud import db, InstanceInfo, MandelboxInfo, RegionToAmi
from app.utils.stripe.payments import payment_required
from app.constants.mandelbox_assign_error_names import MandelboxAssignError

aws_mandelbox_bp = Blueprint("aws_mandelbox_bp", __name__)


@aws_mandelbox_bp.route("/mandelbox/assign", methods=("POST",))
@limiter.limit(RATE_LIMIT_PER_MINUTE)  # type: ignore
@whist_pre_process
@jwt_required()  # type: ignore
@payment_required
@validate()  # type: ignore
def aws_mandelbox_assign(body: MandelboxAssignBody, **_kwargs: Any) -> Tuple[Response, HTTPStatus]:
    start_time = time.time() * 1000
    care_about_active = False
    username = get_jwt_identity()

    # Note: we receive the email from the client, so its value should
    # not be trusted for anything else other than logging since
    # it can be spoofed. We sanitize the email before using to help mitigate
    # potential attacks.
    unsafe_email = sanitize_email(body.user_email)

    if care_about_active and is_user_active(username):
        # If the user already has a mandelbox running, don't start up a new one
        whist_logger.debug(
            f"Returning 503 to user {username} because they are already active. (client reported email {unsafe_email}, this value might not be accurate and is untrusted)"
        )
        return jsonify({"ip": "None", "mandelbox_id": "None"}), HTTPStatus.SERVICE_UNAVAILABLE

    # Of the regions provided in the request, filter out the ones that are not active
    enabled_regions = find_enabled_regions()
    enabled_regions = [r.region_name for r in enabled_regions]
    allowed_regions = [r for r in body.regions if r in enabled_regions]

    if not allowed_regions:
        whist_logger.error(
            f"None of the request regions {body.regions} are enabled, enabled regions are {enabled_regions}"
        )
        return (
            jsonify({"ip": "None", "mandelbox_id": "None", "region": "None"}),
            HTTPStatus.SERVICE_UNAVAILABLE,
        )

    region = allowed_regions[0]

    # Override the commit hash if in dev
    if (
        current_app.config["ENVIRONMENT"] == DEVELOPMENT
        or current_app.config["ENVIRONMENT"] == LOCAL
    ) and body.client_commit_hash == CLIENT_COMMIT_HASH_DEV_OVERRIDE:
        # This condition is to accomodate the worflow for developers of client_apps
        # to test their changes without needing to update the development database with
        # commit_hashes on their local machines.
        client_commit_hash = (
            RegionToAmi.query.filter_by(region_name=region, ami_active=True)
            .one_or_none()
            .client_commit_hash
        )
    else:
        client_commit_hash = body.client_commit_hash

    # Begin finding an instance
    whist_logger.debug(
        f"Trying to find instance for user {username} in region {region},\
        with commit hash {client_commit_hash}. (client reported email {unsafe_email}, this value might not be accurate and is untrusted)"
    )
    instance_or_error = find_instance(region, client_commit_hash)
    time_when_instance_found = time.time() * 1000

    # How long did it take to find an instance?
    time_to_find_instance = time_when_instance_found - start_time
    whist_logger.debug(f"It took {time_to_find_instance} ms to find an instance.")

    if MandelboxAssignError.contains(instance_or_error):
        whist_logger.info(
            f"No instance found in region {region}\
              with client_commit_hash: {client_commit_hash} because {instance_or_error}"
        )

        if not current_app.testing:
            # If we're not testing, we want to scale up a new instance to handle this load
            # and we know what instance type we're missing from the request
            ami = RegionToAmi.query.get(
                {"region_name": body.regions[0], "client_commit_hash": client_commit_hash}
            )
            if ami is None:
                whist_logger.debug(
                    f"No AMI found for region: {region}, commit hash: {client_commit_hash}"
                )
            else:
                scaling_thread = Thread(
                    target=do_scale_up_if_necessary,
                    args=(region, ami.ami_id),
                    kwargs={
                        "flask_app": current_app._get_current_object()  # pylint: disable=protected-access
                    },
                )
                scaling_thread.start()
        whist_logger.debug(
            f"Returning 503 to user {username} because we didn't find an instance for them. (client reported email {unsafe_email}, this value might not be accurate and is untrusted)"
        )
        return (
            jsonify(
                {"ip": "None", "mandelbox_id": "None", "region": region, "error": instance_or_error}
            ),
            HTTPStatus.SERVICE_UNAVAILABLE,
        )

    instance = InstanceInfo.query.get(instance_or_error)
    mandelbox_id = str(uuid.uuid4())
    obj = MandelboxInfo(
        mandelbox_id=mandelbox_id,
        instance_name=instance.instance_name,
        user_id=username,
        session_id=str(body.session_id),
        status="ALLOCATED",
        creation_time_utc_unix_ms=int(time.time() * 1000),
    )
    db.session.add(obj)
    db.session.commit()
    time_when_container_created = time.time() * 1000
    # How long did it take to create a new container row?
    time_to_create_row = time_when_container_created - time_when_instance_found
    whist_logger.debug(f"It took {time_to_create_row} ms to create a container row")
    if not current_app.testing:
        # If we're not testing, we want to scale new instances in the background.
        # Specifically, we want to scale in the region/AMI pair where we know
        # there's usage -- so we call do_scale_up with the location and AMI of the request

        scaling_thread = Thread(
            target=do_scale_up_if_necessary,
            args=(
                region,
                RegionToAmi.query.get(
                    {"region_name": region, "client_commit_hash": client_commit_hash}
                ).ami_id,
            ),
            kwargs={
                "flask_app": current_app._get_current_object()  # pylint: disable=protected-access
            },
        )
        scaling_thread.start()
    # How long did the request take?
    total_request_time = time.time() * 1000 - start_time
    whist_logger.debug(f"In total, this request took {total_request_time} ms to fulfill")
    app_record_metrics(
        metrics={
            "web.time_to_find_instance": time_to_find_instance,
            "web.time_to_create_row": time_to_create_row,
            "web.total_request_time": total_request_time,
        },
        extra_dims={"task_name": "assign_mandelbox"},
    )
    return jsonify({"ip": instance.ip, "mandelbox_id": mandelbox_id}), HTTPStatus.ACCEPTED
