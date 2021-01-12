from flask import Blueprint
from flask.json import jsonify
from flask_jwt_extended import jwt_required

from app import fractal_pre_process
from app.celery.aws_ecs_creation import (
    assign_container,
    create_new_cluster,
    create_new_container,
    send_commands,
)
from app.celery.aws_ecs_deletion import delete_cluster, delete_container, drain_container
from app.constants.http_codes import ACCEPTED, BAD_REQUEST, NOT_FOUND
from app.helpers.blueprint_helpers.aws.aws_container_post import (
    BadAppError,
    ping_helper,
    preprocess_task_info,
    protocol_info,
    set_stun,
)

from app.helpers.utils.general.auth import fractal_auth, developer_required
from app.helpers.utils.locations.location_helper import get_loc_from_ip

aws_container_bp = Blueprint("aws_container_bp", __name__)
allowed_regions = {"us-east-1", "us-east-2", "us-west-1", "us-west-2", "ca-central-1"}


@aws_container_bp.route("/aws_container/<action>", methods=["POST"])
@fractal_pre_process
@jwt_required
@developer_required
def test_endpoint(action, **kwargs):
    """This is an endpoint for administrators and developers to test
    aws container creation, cluster creation, deletion, etcetera. It differs from our
    regular container endpoints in that it requires some different parameters and it
    has some additional functionality (i.e. for clusters).

    Args:
        action (str): The action the user is requiesting in the url
        (look at the route for clarity).

    Returns:
        json, int: the json http response and the http status code
        (which is an int like 200, 400, ...).
    """
    if action == "create_cluster":
        cluster_name, instance_type, ami, region_name, max_size, min_size = (
            kwargs["body"]["cluster_name"],
            kwargs["body"]["instance_type"],
            kwargs["body"]["ami"],
            kwargs["body"]["region_name"],
            kwargs["body"]["max_size"],
            kwargs["body"]["min_size"],
        )
        task = create_new_cluster.apply_async(
            [cluster_name, instance_type, ami, region_name, min_size, max_size]
        )

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED

    if action == "delete_cluster":
        cluster, region_name = (
            kwargs["body"]["cluster_name"],
            kwargs["body"]["region_name"],
        )
        task = delete_cluster.apply_async([cluster, region_name])

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED

    if action == "create_container":
        (username, cluster_name, region_name, task_definition_arn) = (
            kwargs["body"]["username"],
            kwargs["body"]["cluster_name"],
            kwargs["body"].get("region_name", None),
            kwargs["body"]["task_definition_arn"],
        )

        region_name = region_name if region_name else get_loc_from_ip(kwargs["received_from"])
        task = create_new_container.apply_async(
            [username, task_definition_arn],
            {
                "cluster_name": cluster_name,
                "region_name": region_name,
                "webserver_url": kwargs["webserver_url"],
            },
        )
        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED

    if action == "assign_container":
        (username, cluster_name, region_name, task_definition_arn) = (
            kwargs["body"]["username"],
            kwargs["body"]["cluster_name"],
            kwargs["body"].get("region_name", None),
            kwargs["body"]["task_definition_arn"],
        )
        region_name = region_name if region_name else get_loc_from_ip(kwargs["received_from"])
        task = assign_container.apply_async(
            [username, task_definition_arn],
            {
                "cluster_name": cluster_name,
                "region_name": region_name,
                "webserver_url": kwargs["webserver_url"],
            },
        )
        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED

    if action == "send_commands":
        cluster, region_name, commands, containers = (
            kwargs["body"]["cluster"],
            kwargs["body"]["region_name"],
            kwargs["body"]["commands"],
            kwargs["body"].get("containers"),
        )
        task = send_commands.apply_async([cluster, region_name, commands, containers])

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED

    return jsonify({"error": NOT_FOUND}), NOT_FOUND


@aws_container_bp.route("/container/delete", methods=("POST",))
@fractal_pre_process
def aws_container_delete(**kwargs):
    """
    Delete a container. Needs:
    - container_id (str): The ARN of the running container.
    - private_key (str): AES key

    Returns celery ID to poll.
    """
    body = kwargs.pop("body")

    try:
        args = (body.pop("container_id"), body.pop("private_key").lower())
    except (AttributeError, KeyError):
        response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
    else:
        task = delete_container.delay(*args)
        response = jsonify({"ID": task.id}), ACCEPTED

    return response


@aws_container_bp.route("/container/protocol_info", methods=("POST",))
@fractal_pre_process
def aws_container_info(**kwargs):
    """
    Get container info. Needs:
    - identifier (int): the port corresponding to port 32262
    - private_key (str): aes encryption key

    Returns info after a db lookup.
    """
    body = kwargs.pop("body")
    address = kwargs.pop("received_from")

    try:
        identifier = body.pop("identifier")
        private_key = body.pop("private_key")
        private_key = private_key.lower()
    except KeyError:
        response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
    else:
        info, status = protocol_info(address, identifier, private_key)

        if info:
            response = jsonify(info), status
        else:
            response = jsonify({"status": status}), status

    return response


@aws_container_bp.route("/container/ping", methods=("POST",))
@fractal_pre_process
def aws_container_ping(**kwargs):
    """
    Ping aws container. Needs:
    - available (bool): True if Container is not being used, False otherwise
    - identifier (int): the port corresponding to port 32262
    - private_key (str): aes encryption key

    Returns container status
    """
    body = kwargs.pop("body")
    address = kwargs.pop("received_from")

    try:
        available = body.pop("available")
        identifier = body.pop("identifier")
        private_key = body.pop("private_key")
        private_key = private_key.lower()
    except KeyError:
        response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
    else:
        # Update container status.
        data, status = ping_helper(available, address, identifier, private_key)
        response = jsonify(data), status

    return response


@aws_container_bp.route("/container/<action>", methods=["POST"])
@fractal_pre_process
@jwt_required
@fractal_auth
def aws_container_post(action, **kwargs):
    """
    General aws container post. Handles:
    - create
    - assign
    - delete
    - drain
    - stun
    """
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
                dpi = body.get("dpi", 96)
            except KeyError:
                response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
            else:
                # Create a container.
                try:
                    task_arn, _, sample_cluster = preprocess_task_info(app)
                except BadAppError:
                    response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
                else:
                    task = create_new_container.delay(
                        user,
                        task_arn,
                        region_name=region,
                        cluster_name=sample_cluster,
                        webserver_url=kwargs["webserver_url"],
                        dpi=dpi,
                    )
                    response = jsonify({"ID": task.id}), ACCEPTED
        elif action == "assign":
            try:
                app = body.pop("app")
                region = body.pop("region")
                dpi = body.get("dpi", 96)
                if region not in allowed_regions:
                    response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
                    return response
            except KeyError:
                response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
            else:
                # assign a container.
                try:
                    task_arn, _, _ = preprocess_task_info(app)
                except BadAppError:
                    response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
                else:
                    task = assign_container.delay(
                        user,
                        task_arn,
                        region_name=region,
                        webserver_url=kwargs["webserver_url"],
                        dpi=dpi,
                    )
                    response = jsonify({"ID": task.id}), ACCEPTED

        elif action == "delete":
            try:
                container = body.pop("container_id")
            except KeyError:
                response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
            else:
                # Delete the container
                task = delete_container.delay(user, container)
                response = jsonify({"ID": task.id}), ACCEPTED

        elif action == "drain":
            try:
                container = body.pop("container_id")
            except KeyError:
                response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
            else:
                # Delete the container
                task = drain_container.delay(user, container)
                response = jsonify({"ID": task.id}), ACCEPTED

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
