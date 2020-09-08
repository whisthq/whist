from sqlalchemy.exc import DBAPIError

from app import db
from app.constants.http_codes import NOT_FOUND, BAD_REQUEST, SUCCESS
from app.helpers.utils.aws.aws_resource_locks import lockContainerAndUpdate
from app.helpers.utils.general.logs import fractalLog
from app.helpers.utils.general.sql_commands import fractalSQLCommit
from app.helpers.utils.general.sql_commands import fractalSQLUpdate
from app.helpers.utils.general.time import dateToUnix, getToday
from app.models.hardware import UserContainer
from app.models.logs import LoginHistory
from app.helpers.utils.stripe.stripe_payments import stripeChargeHourly


def pingHelper(available, container_ip, version=None):
    """Stores ping timestamps in the v_ms table and tracks number of hours used

    Args:
        available (bool): True if Container is not being used, False otherwise
        container_ip (str): Container IP address

    Returns:
        json: Success/failure
    """

    # Retrieve Container data based on Container IP

    container_info = UserContainer.query.filter_by(ip=container_ip).first()

    if container_info:
        username = container_info.user_id
    else:
        return {"status": BAD_REQUEST}

    fractalLog(function="", label="", logs=str(username))

    fractalSQLCommit(
        db, fractalSQLUpdate, container_info, {"last_pinged": dateToUnix(getToday())}
    )

    # Update container_info version

    if version:
        fractalSQLCommit(db, fractalSQLUpdate, container_info, {"version": version})

    # Define states where we don't change the Container state

    intermediate_states = ["STOPPING", "DEALLOCATING", "ATTACHING"]

    # Detect and handle disconnect event

    if container_info.state == "RUNNING_UNAVAILABLE" and available:

        # Add pending charge if the user is an hourly subscriber

        stripeChargeHourly(username)

        # Add logoff event to timetable

        log = LoginHistory(
            user_id=username, action="logoff", timestamp=dateToUnix(getToday()),
        )

        fractalSQLCommit(db, lambda db, x: db.session.add(x), log)

        fractalLog(
            function="pingHelper",
            label=str(username),
            logs="{username} just disconnected from their cloud PC".format(
                username=username
            ),
        )

    # Detect and handle logon event

    if container_info.state == "RUNNING_AVAILABLE" and not available:

        # Add logon event to timetable

        log = LoginHistory(
            user_id=username, action="logon", timestamp=dateToUnix(getToday()),
        )

        fractalSQLCommit(db, lambda db, x: db.session.add(x), log)

        fractalLog(
            function="pingHelper",
            label=str(username),
            logs="{username} just connected to their cloud PC".format(
                username=username
            ),
        )

    # Change Container states accordingly

    if not container_info.state in intermediate_states and not available:
        lockContainerAndUpdate(
            container_name=container_info.container_id,
            state="RUNNING_UNAVAILABLE",
            lock=True,
            temporary_lock=0,
        )

    if not container_info.state in intermediate_states and available:
        lockContainerAndUpdate(
            container_name=container_info.container_id,
            state="RUNNING_AVAILABLE",
            lock=False,
            temporary_lock=0,
        )

    return {"status": SUCCESS}


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
