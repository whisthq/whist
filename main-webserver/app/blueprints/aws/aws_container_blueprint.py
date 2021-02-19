from flask import abort, Blueprint, make_response, request
from flask.json import jsonify
from flask_jwt_extended import jwt_required

from app import fractal_pre_process
from app.celery.aws_ecs_creation import (
    assign_container,
    create_new_cluster,
    send_commands,
)
from app.helpers.blueprint_helpers.aws.container_state import set_container_state
from app.celery.aws_ecs_deletion import (
    delete_auto_scaling_groups,
    delete_capacity_providers,
    delete_container,
    deregister_container_instances,
)
from app.celery.aws_ecs_deletion import delete_cluster as _delete_cluster
from app.celery.aws_ecs_modification import update_region
from app.constants.container_state_values import CANCELLED
from app.constants.http_codes import ACCEPTED, BAD_REQUEST, NOT_FOUND, SUCCESS
from app.helpers.blueprint_helpers.aws.aws_container_post import (
    BadAppError,
    ping_helper,
    preprocess_task_info,
    protocol_info,
    set_stun,
)
from app.models import ClusterInfo, db

from app.helpers.utils.general.auth import fractal_auth, developer_required, payment_required
from app.helpers.utils.locations.location_helper import get_loc_from_ip
from app.helpers.utils.general.limiter import limiter, RATE_LIMIT_PER_MINUTE


aws_container_bp = Blueprint("aws_container_bp", __name__)


@aws_container_bp.route("/container_state/<action>", methods=["POST"])
@fractal_pre_process
@jwt_required
@fractal_auth
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
    if action == "cancel":
        body = kwargs.pop("body")
        try:
            user = body.pop("username")
            task = body.pop("task")
            set_container_state(keyuser=user, keytask=task, state=CANCELLED)
        except:
            return jsonify({"status": BAD_REQUEST}), BAD_REQUEST
        return jsonify({"status": SUCCESS}), SUCCESS
    return jsonify({"error": NOT_FOUND}), NOT_FOUND


@aws_container_bp.route("/aws_container/delete_cluster", methods=("POST",))
@jwt_required
@developer_required
def delete_cluster():
    """Delete the specified cluster.

    POST keys:
        cluster_name: The short name of the cluster to delete as a string.
        region_name: The name of the AWS region in which the cluster is running as a string.
        force: Optional. A boolean indiciating whether or not to delete the cluster even if there
            are still ECS tasks running on the cluster.

    Returns:
        A dictionary containing a single key, "ID", whose value is the task ID of a Celery task
        whose progress may be monitored through the /status/ endpoint.
    """

    # Make sure the POST body is formatted as either form data or JSON, and that it contains the
    # keys "cluster_name" and "region_name"
    try:
        if request.is_json:
            body = request.json
        else:
            body = request.form

        cluster_name = body["cluster_name"]
        region = body["region_name"]
    except KeyError:
        abort(400)

    force = body.get("force", False)
    cluster = ClusterInfo.query.with_for_update().get(cluster_name)

    if cluster is None:
        abort((make_response({"error": f"The cluster {cluster_name} does not exist."}, 400)))

    if cluster.containers.count() > 0 and not force:
        abort((make_response({"error": f"There are still tasks running on {cluster_name}."}, 400)))

    db.session.delete(cluster)
    db.session.commit()

    task = (
        deregister_container_instances.s(cluster_name, region)
        | _delete_cluster.si(cluster_name, region)
        | delete_capacity_providers.s(region)
        | delete_auto_scaling_groups.s(region)
    )()

    return make_response({"ID": task.id}), ACCEPTED


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
        try:
            cluster_name, instance_type, ami, region_name, max_size, min_size = (
                kwargs["body"]["cluster_name"],
                kwargs["body"]["instance_type"],
                kwargs["body"].get("ami", None),
                kwargs["body"]["region_name"],
                kwargs["body"].get("max_size", 10),
                kwargs["body"].get("min_size", 1),
            )
        except KeyError:
            return jsonify({"ID": None}), BAD_REQUEST

        task = create_new_cluster.apply_async(
            [cluster_name, instance_type, ami, region_name, min_size, max_size]
        )

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED

    if action == "update_region":
        ami, region_name = (
            kwargs["body"].get("ami", None),
            kwargs["body"]["region_name"],
        )
        task = update_region.delay(ami=ami, region_name=region_name)

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED

    if action == "assign_container":
        try:
            (username, cluster_name, region_name, task_definition_arn) = (
                kwargs["body"]["username"],
                kwargs["body"]["cluster_name"],
                kwargs["body"]["region_name"],
                kwargs["body"]["task_definition_arn"],
            )
        except KeyError:
            return jsonify({"ID": None}), BAD_REQUEST

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


@aws_container_bp.route("/container/assign", methods=("POST",))
@limiter.limit(RATE_LIMIT_PER_MINUTE)
@fractal_pre_process
@jwt_required
@fractal_auth
@payment_required
def aws_container_assign(**kwargs):
    """
    Assigns aws container. Needs:
    - username (str): username
    - app (str): name of app that user is trying to use
    - region (str): region in which to host in AWS
    - dpi (int): dots per inch

    Returns container status
    """
    response = jsonify({"status": NOT_FOUND}), NOT_FOUND
    body = kwargs.pop("body")

    try:
        user = body.pop("username")
        app = body.pop("app")
        region = body.pop("region")
        dpi = body.get("dpi", 96)
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

    return response


@aws_container_bp.route("/container/<action>", methods=["POST"])
@fractal_pre_process
@jwt_required
@fractal_auth
def aws_container_post(action, **kwargs):
    """
    General aws container post. Handles:
    - delete
    - stun
    """
    response = jsonify({"status": NOT_FOUND}), NOT_FOUND
    body = kwargs.pop("body")

    try:
        user = body.pop("username")
    except KeyError:
        response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
    else:
        if action == "delete":
            try:
                container = body.pop("container_id")
            except KeyError:
                response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
            else:
                # Delete the container
                task = delete_container.delay(user, container)
                response = jsonify({"ID": task.id}), ACCEPTED

        if action == "stun":
            try:
                container_id = body.pop("container_id")
                using_stun = body.pop("stun")
            except KeyError:
                response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
            else:
                status = set_stun(user, container_id, using_stun)
                response = jsonify({"status": status}), status

    return response
