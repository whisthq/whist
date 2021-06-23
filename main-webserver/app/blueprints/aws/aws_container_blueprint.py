from threading import Thread
import uuid
from flask import Blueprint, current_app
from flask.json import jsonify
from flask_jwt_extended import get_jwt_identity, jwt_required
from flask_pydantic import validate
from app.validation import MandelboxAssignBody

from app import fractal_pre_process, log_request
from app.helpers.blueprint_helpers.aws.container_state import (
    set_container_state,
)
from app.constants.container_state_values import CANCELLED
from app.constants.http_codes import (
    ACCEPTED,
    BAD_REQUEST,
    NOT_FOUND,
    RESOURCE_UNAVAILABLE,
    SUCCESS,
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


@aws_container_bp.route("/container_state/<action>", methods=["POST"])
@fractal_pre_process
@jwt_required()
def container_state(action, **kwargs):
    """This is an endpoint which accesses our container_state_values table.
    This table hides much of the inner workings of container spinup from users
    and simply provides information regarding whether a user's app/container is
    ready, cancelled, failed, etc. It is meant to be listened on through
    a GraphQL websocket (subscription) for the loading page in the client app.

    Args:
        action (str): The string at the end of the URL which signifies
        which action we want to take. Currently, only the cancel endpoint is provided,
        which allows us to easily cancel a spinning up container and clean up our redux state.

    Returns:
        json, int: A json HTTP response and status code.
    """
    user = get_jwt_identity()

    if action == "cancel":
        body = kwargs.pop("body")
        try:
            task = body.pop("task")
            set_container_state(keyuser=user, keytask=task, state=CANCELLED)
        except:
            return jsonify({"status": BAD_REQUEST}), BAD_REQUEST
        return jsonify({"status": SUCCESS}), SUCCESS
    return jsonify({"error": NOT_FOUND}), NOT_FOUND


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
    instance_id = find_instance(body.region)
    if instance_id is None:

        if not current_app.testing:
            # If we're not testing, we want to scale up a new instance to handle this load
            # and we know what instance type we're missing from the request
            scaling_thread = Thread(
                target=do_scale_up_if_necessary,
                args=(body.region, RegionToAmi.query.get(body.region).ami_id),
            )
            scaling_thread.start()
        return jsonify({"IP": "None"}), RESOURCE_UNAVAILABLE

    instance = InstanceInfo.query.get(instance_id)
    obj = ContainerInfo(
        container_id=str(uuid.uuid4()),
        instance_name=instance.instance_name,
        user_id=body.username,
        status="ALLOCATED",
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

    return jsonify({"IP": instance.ip}), ACCEPTED
