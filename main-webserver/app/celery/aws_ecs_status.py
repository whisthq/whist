from app import celery_instance, db
from app.constants.http_codes import SUCCESS, UNAUTHORIZED
from app.helpers.utils.aws.aws_resource_locks import lockContainerAndUpdate
from app.helpers.utils.general.logs import fractalLog
from app.helpers.utils.general.sql_commands import fractalSQLCommit
from app.helpers.utils.general.sql_commands import fractalSQLUpdate
from app.helpers.utils.general.time import dateToUnix, getToday
from app.helpers.utils.stripe.stripe_payments import stripeChargeHourly
from app.models.hardware import UserContainer
from app.models.logs import LoginHistory


@celery_instance.task
def pingHelper(available, container_ip, port_32262, aeskey, version=None):
    """Stores ping timestamps in the v_ms table and tracks number of hours used

    Args:
        port_32262(int): the port corresponding to port 32262
        available (bool): True if Container is not being used, False otherwise
        container_ip (str): Container IP address

    Returns:
        json: Success/failure
    """

    # Retrieve Container data based on Container IP

    container_info = UserContainer.query.filter_by(ip=container_ip, port_32262=port_32262).first()

    if container_info:
        username = container_info.user_id
    else:
        raise Exception(f"No container with IP {container_ip} and ports {[port_32262]}")

    if container_info.secret_key != aeskey:
        fractalLog(
            function="pingHelper",
            label=str(container_info.container_name),
            logs=f"{container_info.container_name} just tried to ping with the wrong AES key, using\
{aeskey} when {container_info.secret_key} was expected",
        )
        return {"status": UNAUTHORIZED}

    fractalSQLCommit(db, fractalSQLUpdate, container_info, {"last_pinged": dateToUnix(getToday())})

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
            lock=True,
            temporary_lock=0,
        )

    if container_info.state not in intermediate_states and available:
        lockContainerAndUpdate(
            container_name=container_info.container_id,
            state="RUNNING_AVAILABLE",
            lock=False,
            temporary_lock=0,
        )

    return {"status": SUCCESS}
