from flask import Blueprint
from flask.json import jsonify
from flask_jwt_extended import jwt_required

from app import fractalPreProcess
from app.celery.aws_ecs_creation import BadAppError, create_new_container
from app.celery.aws_ecs_deletion import deleteContainer
from app.constants.http_codes import ACCEPTED, BAD_REQUEST, NOT_FOUND
from app.helpers.blueprint_helpers.aws.aws_container_post import pingHelper
from app.helpers.utils.general.auth import fractalAuth

aws_container_bp = Blueprint("aws_container_bp", __name__)


@aws_container_bp.route("/aws_container/<action>", methods=["POST"])
@fractalPreProcess
def test_endpoint(action, **kwargs):
    if action == "create_cluster":
        instance_type, ami = (
            kwargs["body"]["instance_type"],
            kwargs["body"]["ami"],
        )
        task = create_new_cluster.apply_async([instance_type, ami])

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED

    if action == "launch_task":
        username, cluster_name = (
            kwargs["body"]["username"],
            kwargs["body"]["cluster_name"],
        )
        task = create_new_container.apply_async([username, cluster_name])

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
            # TODO: Toggle whether a container uses stun. Inspire yourself from the disk/stun endpoint. You'll need to
            # create a using_stun column in the user_containers table, which you can do from TablePlus. When you create
            # a new column, make sure to modify the appropriate class in the /models folder.
            pass

        elif action == "protocol_info":
            # TODO: Same as /stun, migrate over disk/protocol_info
            pass

    return response
