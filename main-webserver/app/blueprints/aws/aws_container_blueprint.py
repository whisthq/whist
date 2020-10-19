from flask import Blueprint
from flask.json import jsonify

from app import fractalPreProcess
from app.celery.aws_ecs_creation import create_new_cluster, create_new_container, send_commands
from app.celery.aws_ecs_deletion import delete_cluster, deleteContainer, drainContainer
from app.celery.aws_ecs_status import pingHelper
from app.constants.http_codes import ACCEPTED, BAD_REQUEST, NOT_FOUND
from app.helpers.blueprint_helpers.aws.aws_container_post import (
    BadAppError,
    preprocess_task_info,
    protocol_info,
    set_stun,
)

from app.helpers.utils.general.auth import fractalAuth
from app.helpers.utils.locations.location_helper import get_loc_from_ip
from flask_jwt_extended import jwt_required

aws_container_bp = Blueprint("aws_container_bp", __name__)


@aws_container_bp.route("/aws_container/<action>", methods=["POST"])
@fractalPreProcess
def test_endpoint(action, **kwargs):
    if action == "create_cluster":
        cluster_name, instance_type, ami, region_name, max_containers, min_containers = (
            kwargs["body"]["cluster_name"],
            kwargs["body"]["instance_type"],
            kwargs["body"]["ami"],
            kwargs["body"]["region_name"],
            kwargs["body"]["max_containers"],
            kwargs["body"]["min_containers"],
        )
        task = create_new_cluster.apply_async(
            [cluster_name, instance_type, ami, region_name, min_containers, max_containers]
        )

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED

    if action == "delete_cluster":
        cluster, region_name = (
            kwargs["body"]["cluster"],
            kwargs["body"]["region_name"],
        )
        task = delete_cluster.apply_async([cluster, region_name])

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED

    if action == "create_container":
        (
            username,
            cluster_name,
            region_name,
            task_definition_arn,
            use_launch_type,
            network_configuration,
        ) = (
            kwargs["body"]["username"],
            kwargs["body"]["cluster_name"],
            kwargs["body"].get("region_name", None),
            kwargs["body"]["task_definition_arn"],
            kwargs["body"]["use_launch_type"],
            kwargs["body"]["network_configuration"],
        )
        region_name = region_name if region_name else get_loc_from_ip(kwargs["received_from"])
        task = create_new_container.apply_async(
            [username, task_definition_arn],
            {
                "cluster_name": cluster_name,
                "region_name": region_name,
                "use_launch_type": use_launch_type,
                "network_configuration": network_configuration,
            },
        )
        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED

    if action == "delete_container":
        user_id, container_name = (
            kwargs["body"]["user_id"],
            kwargs["body"]["container_name"],
        )
        task = deleteContainer.apply_async([user_id, container_name])

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED

    if action == "delete_container":
        user_id, container_name = (
            kwargs["body"]["user_id"],
            kwargs["body"]["container_name"],
        )
        task = deleteContainer.apply_async([user_id, container_name])

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED

    if action == "send_commands":
        cluster, region_name, commands, containers = (
            kwargs["body"]["cluster"],
            kwargs["body"]["region_name"],
            kwargs["body"]["commands"],
            kwargs["body"]["containers"],
        )
        task = send_commands.apply_async([cluster, region_name, commands, containers])

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED


@aws_container_bp.route("/container/protocol_info", methods=("POST",))
@fractalPreProcess
def aws_container_info(**kwargs):
    body = kwargs.pop("body")
    address = kwargs.pop("received_from")
    try:
        identifier = body.pop("identifier")
    except KeyError:
        response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
        return response
    else:
        private_key = body.pop("private_key")
        info, status = protocol_info(address, identifier, private_key)

    if info is not None:
        response = jsonify(info), status
    else:
        response = jsonify({"status": status}), status

    return response


@aws_container_bp.route("/container/ping", methods=("POST",))
@fractalPreProcess
def aws_container_ping(**kwargs):
    body = kwargs.pop("body")
    address = kwargs.pop("received_from")

    try:
        available = body.pop("available")
        identifier = body.pop("identifier")
        private_key = body.pop("private_key")
    except KeyError:
        response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
    else:
        # Update container status.
        status = pingHelper.delay(available, address, identifier, private_key)
        response = jsonify(status), status["status"]

    return response


@aws_container_bp.route("/container/<action>", methods=["POST"])
@fractalPreProcess
@jwt_required
@fractalAuth
def aws_container_post(action, **kwargs):
    response = jsonify({"status": NOT_FOUND}), NOT_FOUND
    body = kwargs.pop("body")

    try:
        user = body.pop("username")

    except KeyError:
        response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
    else:
        if action == "create":
            # Access required keys.
            try:
                app = body.pop("app")
                region = body.pop("region")
            except KeyError:
                response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
            else:
                # Create a container.
                try:
                    task_arn, _, sample_cluster = preprocess_task_info(app)
                except BadAppError:
                    response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
                else:
                    task = create_new_container.delay(user, task_arn, region, cluster_name=sample_cluster)
                    response = jsonify({"ID": task.id}), ACCEPTED

        elif action == "delete":
            try:
                container = body.pop("container_id")
            except KeyError:
                response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
            else:
                # Delete the container
                task = deleteContainer.delay(user, container)
                response = jsonify({"ID": task.id}), ACCEPTED

        elif action == "drain":
            try:
                container = body.pop("container_id")
            except KeyError:
                response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
            else:
                # Delete the container
                task = drainContainer.delay(user, container)
                response = jsonify({"ID": task.id}), ACCEPTED

        elif action == "restart":
            # TODO: Same as above, but inspire yourself from the vm/restart endpoint.
            pass

        elif action == "stun":
            try:
                container_id = body.pop("container_id")
                using_stun = body.pop("stun")
            except KeyError:
                response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
            else:
                status = set_stun(user, container_id, using_stun)
                response = jsonify({"status": status}), status

    return response
