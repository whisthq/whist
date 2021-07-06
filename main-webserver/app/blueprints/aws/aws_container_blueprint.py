from flask import abort, Blueprint
from flask.json import jsonify
from flask_jwt_extended import get_jwt_identity, jwt_required
from sqlalchemy.orm.exc import NoResultFound

from app import fractal_pre_process, log_request
from app.maintenance.maintenance_manager import (
    check_if_maintenance,
    try_start_maintenance,
    try_end_maintenance,
)
from app.celery.aws_ecs_creation import (
    assign_container,
    create_new_cluster,
)
from app.helpers.blueprint_helpers.aws.container_state import set_container_state
from app.celery.aws_ecs_deletion import delete_cluster, delete_container
from app.celery.aws_ecs_modification import (
    update_region,
    update_task_definitions,
)
from app.constants.container_state_values import CANCELLED
from app.constants.http_codes import (
    ACCEPTED,
    BAD_REQUEST,
    NOT_FOUND,
    SUCCESS,
    WEBSERVER_MAINTENANCE,
)
from app.helpers.blueprint_helpers.aws.aws_container_post import (
    BadAppError,
    ping_helper,
    preprocess_task_info,
    protocol_info,
)
from app.helpers.utils.general.auth import developer_required, payment_required
from app.helpers.utils.locations.location_helper import get_loc_from_ip
from app.helpers.utils.general.limiter import limiter, RATE_LIMIT_PER_MINUTE
from app.models import ClusterInfo, RegionToAmi

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


@aws_container_bp.route("/aws_container/delete_cluster", methods=("POST",))
@fractal_pre_process
@jwt_required()
@developer_required
def aws_cluster_delete(**kwargs):
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

    body = kwargs["body"]

    try:
        cluster_name = body["cluster_name"]
        region = body["region_name"]
    except KeyError:
        abort(400)

    force = body.get("force", False)

    try:
        cluster = ClusterInfo.query.filter_by(cluster=cluster_name, location=region).one()
    except NoResultFound:
        return jsonify({"error": f"The cluster {cluster_name} does not exist."}), 400

    task = delete_cluster.delay(cluster, force=force)

    return jsonify({"ID": task.id}), ACCEPTED


@aws_container_bp.route("/aws_container/<action>", methods=["POST"])
@fractal_pre_process
@jwt_required()
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
    # check for maintenance-related things
    if action in ["create_cluster", "assign_mandelbox"]:
        if check_if_maintenance():
            # server cannot be in maintenance mode to do this
            return (
                jsonify(
                    {
                        "error": "Webserver is in maintenance mode.",
                    }
                ),
                WEBSERVER_MAINTENANCE,
            )
    elif action in ["update_region", "update_taskdefs"]:
        if not check_if_maintenance():
            # server must be in maintenance mode to do this
            return (
                jsonify(
                    {
                        "error": "Webserver must be put in maintenance mode before doing this.",
                    }
                ),
                BAD_REQUEST,
            )

    # handle the action. The general design pattern is to parse the arguments relevant to
    # the action and start a celery task to handle it. `start_maintenance` and `end_maintenance`
    # are the only actions that run synchronously.
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

        task = create_new_cluster.delay(
            cluster_name,
            instance_type,
            ami,
            region_name,
            min_size,
            max_size,
        )

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED

    if action == "update_region":
        ami, region_name, webserver_url = (
            kwargs["body"].get("ami", None),
            kwargs["body"]["region_name"],
            kwargs["webserver_url"],
        )
        task = update_region.delay(webserver_url=webserver_url, region_name=region_name, ami=ami)

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED

    if action == "update_taskdefs":
        # update the task definition numbers for fractal's supported app images
        # payload can optionally contain `app_id` and `task_version`.
        # see `update_task_definitions` for how these are used and the behavior
        # of this endpoint. returns a celery task ID.
        app_id, task_version = (
            kwargs["body"].get("app_id", None),
            kwargs["body"].get("task_version", None),
        )
        task = update_task_definitions.delay(app_id=app_id, task_version=task_version)
        if not task:
            return jsonify({"ID": None}), BAD_REQUEST
        return jsonify({"ID": task.id}), ACCEPTED

    if action == "assign_mandelbox":
        try:
            # TODO: do request validation like in /mandelbox/assign
            (username, cluster_name, region_name, task_definition_arn, task_version) = (
                kwargs["body"]["username"],
                kwargs["body"]["cluster_name"],
                kwargs["body"]["region_name"],
                kwargs["body"]["task_definition_arn"],
                kwargs["body"].get("task_version", None),
            )
        except KeyError:
            return jsonify({"ID": None}), BAD_REQUEST
        region_name = region_name if region_name else get_loc_from_ip(kwargs["received_from"])

        task = assign_container.delay(
            username=username,
            task_definition_arn=task_definition_arn,
            task_version=task_version,
            region_name=region_name,
            cluster_name=cluster_name,
            webserver_url=kwargs["webserver_url"],
        )

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST
        return jsonify({"ID": task.id}), ACCEPTED

    if action == "start_maintenance":
        # synchronously try to put the webserver into maintenance mode.
        # return is a dict {"success": <bool>, "msg": <str>}. success
        # is only True if webserver is in maintenance mode. msg is
        # human-readable and tells the client what happened.
        success, msg = try_start_maintenance()
        return jsonify({"success": success, "msg": msg}), SUCCESS

    if action == "end_maintenance":
        # synchronously try to end maintenance mode.
        # return is a dict {"success": <bool>, "msg": <str>}. success
        # is only True if webserver has ended maintenance mode. msg is
        # human-readable and tells the client what happened.
        success, msg = try_end_maintenance()
        return jsonify({"success": success, "msg": msg}), SUCCESS

    return jsonify({"error": NOT_FOUND}), NOT_FOUND


@aws_container_bp.route("/mandelbox/delete", methods=("POST",))
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


@aws_container_bp.route("/mandelbox/protocol_info", methods=("POST",))
@fractal_pre_process
def aws_container_info(**kwargs):
    """
    Get container info. Needs:
    - identifier (int): the port corresponding to port 32262
    - private_key (str): aes encryption key

    Returns info after a db lookup.
    """
    body = kwargs.pop("body")
    address = body.get("ip", kwargs.pop("received_from"))

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


@aws_container_bp.route("/mandelbox/ping", methods=("POST",))
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


@aws_container_bp.route("/mandelbox/assign", methods=("POST",))
@limiter.limit(RATE_LIMIT_PER_MINUTE)
@fractal_pre_process
@jwt_required()
@payment_required
def aws_container_assign(**kwargs):
    """
    Assigns aws container. Needs:
    - username (str): username
    - config_encryption_token (str): the encryption token for app config
    - app (str): name of app that user is trying to use
    - region (str): region in which to host in AWS
    - dpi (int): dots per inch

    Returns container status
    """
    body = kwargs.pop("body")
    response = jsonify({"status": NOT_FOUND}), NOT_FOUND
    user = get_jwt_identity()

    try:
        app = body.pop("app")
        region = body.pop("region")
        dpi = body.get("dpi", 96)
        if check_if_maintenance():
            return (
                jsonify(
                    {
                        "error": "Webserver is in maintenance mode.",
                    }
                ),
                WEBSERVER_MAINTENANCE,
            )
    except KeyError:
        response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
    else:
        # assign a container.
        try:
            task_arn, task_version, _, _ = preprocess_task_info(app)
        except BadAppError:
            response = jsonify({"status": BAD_REQUEST}), BAD_REQUEST
        else:
            task = assign_container.delay(
                user,
                task_arn,
                task_version,
                region_name=region,
                webserver_url=kwargs["webserver_url"],
                dpi=dpi,
            )
            response = jsonify({"ID": task.id}), ACCEPTED

    return response
