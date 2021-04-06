from sqlalchemy.exc import DBAPIError

from app.constants.container_state_values import CANCELLED, FAILURE
from app.constants.http_codes import (
    NOT_FOUND,
    BAD_REQUEST,
    SUCCESS,
    UNAUTHORIZED,
)
from app.helpers.utils.aws.aws_resource_locks import lock_container_and_update
from app.helpers.utils.general.logs import fractal_logger
from app.helpers.utils.general.sql_commands import (
    fractal_sql_commit,
    fractal_sql_update,
)
from app.helpers.utils.general.time import date_to_unix, get_today
from app.models import db, LoginHistory, UserContainerState, UserContainer, SupportedAppImages
from app.serializers.hardware import UserContainerSchema


class BadAppError(Exception):
    """Raised when `preprocess_task_info` doesn't recognized an input."""


def ping_helper(available, container_ip, port_32262, aeskey, version=None):
    """Update container state in the database.

    Args:
        available (bool): True if Container is not being used, False otherwise
        container_ip (str): Container IP address
        port_32262 (int): the port corresponding to port 32262
        aeskey (str): The container's AES key.
        version (str): The version of the protocol that is running in the
            container.

    Returns:
        A pair of HTTP response JSON and an HTTP status code.
    """

    # Retrieve Container data based on Container IP
    container_info = UserContainer.query.filter_by(ip=container_ip, port_32262=port_32262).first()

    if not container_info or container_info.secret_key != aeskey:
        return (
            {"error": f"No container with IP {container_ip} and ports {[port_32262]}"},
            NOT_FOUND,
        )

    username = container_info.user_id

    fractal_sql_commit(
        db, fractal_sql_update, container_info, {"last_pinged": date_to_unix(get_today())}
    )

    # Update container_info version
    if version:
        fractal_sql_commit(db, fractal_sql_update, container_info, {"version": version})

    # Define states where we don't change the Container state
    intermediate_states = ["STOPPING", "DEALLOCATING", "ATTACHING"]

    # Detect and handle disconnect event
    if container_info.state == "RUNNING_UNAVAILABLE" and available:
        # Add logoff event to timetable
        log = LoginHistory(
            user_id=username,
            action="logoff",
            timestamp=date_to_unix(get_today()),
        )

        fractal_sql_commit(db, lambda db, x: db.session.add(x), log)

        fractal_logger.info(
            "{username} just disconnected from Fractal".format(username=username),
            extra={"label": str(username)},
        )

    # Detect and handle logon event
    if container_info.state == "RUNNING_AVAILABLE" and not available:
        # Add logon event to timetable
        log = LoginHistory(
            user_id=username,
            action="logon",
            timestamp=date_to_unix(get_today()),
        )

        fractal_sql_commit(db, lambda db, x: db.session.add(x), log)

        fractal_logger.info(
            "{username} just connected to Fractal".format(username=username),
            extra={"label": str(username)},
        )

    # Change Container states accordingly
    if container_info.state not in intermediate_states and not available:
        lock_container_and_update(
            container_name=container_info.container_id,
            state="RUNNING_UNAVAILABLE",
        )

    if container_info.state not in intermediate_states and available:
        lock_container_and_update(
            container_name=container_info.container_id,
            state="RUNNING_AVAILABLE",
        )

    return {"status": "OK"}, SUCCESS


def preprocess_task_info(app):
    """Maps names of applications to ECS task definitions.

    Arguments:
        app: The name of the application for which to generate a task
            definition.

    Returns:
        A  triple containing arguments two through four of the
            create_new_container celery task.
    """

    # TODO: Don't just hard-code the cluster and region
    app_data = SupportedAppImages.query.filter_by(app_id=app).first()
    if app_data:
        return (
            app_data.task_definition,
            app_data.task_version,
            "us-east-1",
            None,
        )
    raise BadAppError("No Matching App Found")


def protocol_info(address, port, aeskey):
    """Returns information, which is consumed by the protocol, to the client.

    Arguments:
        address: The IP address of the container whose information should be
            returned.
    """

    schema = UserContainerSchema(
        only=(
            "allow_autoupdate",
            "branch",
            "secret_key",
            "using_stun",
            "container_id",
            "user_id",
            "dpi",
            "state",
        )
    )
    response = None, NOT_FOUND
    container = UserContainer.query.filter_by(ip=address, port_32262=port).first()

    if container:
        if container.secret_key == aeskey:
            response = schema.dump(container), SUCCESS
        else:
            response = None, UNAUTHORIZED

    return response


def set_stun(user_id, container_id, using_stun):
    """Updates whether or not the specified container should use STUN.

    Arguments:
        user_id: The user_id of the authenticated user.
        container_id: The container_id of the container to modify.
        using_stun: A boolean indicating whether or not the specified container
            should use STUN.
    """

    status = NOT_FOUND
    container = UserContainer.query.get(container_id)

    if getattr(container, "user_id", None) == user_id:
        assert user_id  # Sanity check

        container.using_stun = using_stun

        try:
            db.session.commit()
        except DBAPIError:
            status = BAD_REQUEST
        else:
            status = SUCCESS

    return status


def check_if_live(user_id: str) -> bool:
    """
    Checks if a given user has a live container running
    Args:
        user_id: the id of the user

    Returns: True if the user has a live container in the db
    """
    return (
        len(
            UserContainerState.query.filter(
                UserContainerState.user_id == user_id,
                UserContainerState.state != FAILURE,
                UserContainerState.state != CANCELLED,
            )
            .limit(1)
            .count()
        )
        > 0
    )
