from sqlalchemy.exc import DBAPIError

from app.constants.http_codes import (
    NOT_FOUND,
    BAD_REQUEST,
    SUCCESS,
    UNAUTHORIZED,
)
from app.helpers.utils.aws.aws_resource_locks import lockContainerAndUpdate
from app.helpers.utils.general.logs import fractalLog
from app.helpers.utils.general.sql_commands import (
    fractalSQLCommit,
    fractalSQLUpdate,
)
from app.helpers.utils.general.time import dateToUnix, getToday
from app.models import db, LoginHistory, UserContainer, SupportedAppImages
from app.serializers.hardware import UserContainerSchema


class BadAppError(Exception):
    """Raised when `preprocess_task_info` doesn't recognized an input."""


def pingHelper(available, container_ip, port_32262, aeskey, version=None):
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

    fractalSQLCommit(db, fractalSQLUpdate, container_info, {"last_pinged": dateToUnix(getToday())})

    # Update container_info version
    if version:
        fractalSQLCommit(db, fractalSQLUpdate, container_info, {"version": version})

    # Define states where we don't change the Container state
    intermediate_states = ["STOPPING", "DEALLOCATING", "ATTACHING"]

    # Detect and handle disconnect event
    if container_info.state == "RUNNING_UNAVAILABLE" and available:
        # Add logoff event to timetable
        log = LoginHistory(
            user_id=username,
            action="logoff",
            timestamp=dateToUnix(getToday()),
        )

        fractalSQLCommit(db, lambda db, x: db.session.add(x), log)

        fractalLog(
            function="pingHelper",
            label=str(username),
            logs="{username} just disconnected from Fractal".format(username=username),
        )

    # Detect and handle logon event
    if container_info.state == "RUNNING_AVAILABLE" and not available:
        # Add logon event to timetable
        log = LoginHistory(
            user_id=username,
            action="logon",
            timestamp=dateToUnix(getToday()),
        )

        fractalSQLCommit(db, lambda db, x: db.session.add(x), log)

        fractalLog(
            function="pingHelper",
            label=str(username),
            logs="{username} just connected to Fractal".format(username=username),
        )

    # Change Container states accordingly
    if container_info.state not in intermediate_states and not available:
        lockContainerAndUpdate(
            container_name=container_info.container_id,
            state="RUNNING_UNAVAILABLE",
            lock=False,
            temporary_lock=0,
        )

    if container_info.state not in intermediate_states and available:
        lockContainerAndUpdate(
            container_name=container_info.container_id,
            state="RUNNING_AVAILABLE",
            lock=False,
            temporary_lock=0,
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
