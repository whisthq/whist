from app import *
from app.helpers.utils.aws.aws_resource_locks import *
from app.helpers.utils.stripe.stripe_payments import *


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