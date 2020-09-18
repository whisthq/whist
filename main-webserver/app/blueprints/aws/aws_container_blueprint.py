from flask import Blueprint
from flask.json import jsonify
from flask_jwt_extended import jwt_required

from app import fractalPreProcess
from app.celery.aws_ecs_creation import BadAppError, create_new_container, create_new_cluster, send_commands
from app.celery.aws_ecs_deletion import deleteContainer, delete_cluster
from app.constants.http_codes import ACCEPTED, BAD_REQUEST, NOT_FOUND
from app.helpers.blueprint_helpers.aws.aws_container_get import protocol_info
from app.helpers.blueprint_helpers.aws.aws_container_post import pingHelper
from app.helpers.blueprint_helpers.aws.aws_container_post import set_stun
from app.helpers.utils.general.auth import fractalAuth

aws_container_bp = Blueprint("aws_container_bp", __name__)


@aws_container_bp.route("/aws_container/<action>", methods=["POST"])
@fractalPreProcess
def test_endpoint(action, **kwargs):
    if action == "create_cluster":
        cluster_name, instance_type, ami, region_name = (
            kwargs["body"]["cluster_name"],
            kwargs["body"]["instance_type"],
            kwargs["body"]["ami"],
            kwargs["body"]["region_name"],
        )
        task = create_new_cluster.apply_async([cluster_name, instance_type, ami, region_name])

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
            kwargs["body"]["region_name"],
            kwargs["body"]["task_definition_arn"],
            kwargs["body"]["use_launch_type"],
            kwargs["body"]["network_configuration"],
        )
        task = create_new_container.apply_async(
            [
                username,
                cluster_name,
                region_name,
                task_definition_arn,
                use_launch_type,
                network_configuration,
            ]
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


@aws_container_bp.route("/container/<action>", methods=["POST"])
@fractalPreProcess
@jwt_required
@fractalAuth
def aws_container_post(action, **kwargs):
    response = jsonify({"error": "Not Found"}), NOT_FOUND
    body = kwargs.pop("body")
    try:
        user = body.pop("username")
    except KeyError:
        response = jsonify({"error": "Bad Request"}), BAD_REQUEST
    else:
        if action == "create":
            # Access required keys.
            try:
                app = body.pop("app")
            except KeyError:
                response = jsonify({"error": "Bad Request"}), BAD_REQUEST
            else:
                # Create a container.
                # TODO: Don't just create every container in us-east-1
                try:
                    task = create_new_container.delay(user, app)
                except BadAppError:
                    response = jsonify({"error": "Bad Request"}), BAD_REQUEST
                else:
                    response = jsonify({"ID": task.id}), ACCEPTED

        elif action == "delete":
            try:
                container = body.pop("container_id")
            except KeyError:
                response = jsonify({"ID": None}), BAD_REQUEST
            else:
                # Delete the container
                task = deleteContainer(container, user)
                response = jsonify({"ID": task.id}), ACCEPTED

        elif action == "restart":
            # TODO: Same as above, but inspire yourself from the vm/restart endpoint.
            pass

        elif action == "ping":
            address = kwargs.pop("received_from")

            try:
                available = body.pop("available")
            except KeyError:
                response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
            else:
                # Update container status.
                status = pingHelper(available, address)
                response = jsonify(status), status["status"]

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


@aws_container_bp.route("/container/<action>")
@fractalPreProcess
def aws_container_get(action, **kwargs):
    response = jsonify({"error": NOT_FOUND}), NOT_FOUND

    if action == "protocol_info":
        address = kwargs.pop("received_from")
        info, status = protocol_info(address)

        if info:
            response = jsonify(info), status
        else:
            response = jsonify({"error": status}), status

    return response
