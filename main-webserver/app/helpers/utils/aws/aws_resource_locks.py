from app.helpers.utils.aws.aws_general import getContainerUser
from app.helpers.utils.azure.azure_general import (
    dateToUnix,
    fractalLog,
    fractalSQLCommit,
    getToday,
    shiftUnixByMinutes,
)
from app.models.hardware import *
import logging
import time


def lockContainerAndUpdate(container_name, state, lock, temporary_lock):
    """Changes the state, lock, and temporary lock of a Container

    Args:
        container_name (str): Name of Container
        state (str): Desired state of Container
            [RUNNING_AVAILABLE, RUNNING_UNAVAILABLE, DEALLOCATED, DEALLOCATING, STOPPED, STOPPING,
            DELETING, CREATING, RESTARTING, STARTING]
        lock (bool): True if Container is locked, False otherwise
        temporary_lock (int): Number of minutes, starting from now, to lock the Container (max is 10)


    Returns:
        int: 1 = container is unlocked, -1 = giving up
    """

    new_params = {"state": state, "lock": lock}

    if not state:
        new_params = {"lock": lock}

    if temporary_lock:
        new_temporary_lock = shiftUnixByMinutes(dateToUnix(getToday()), temporary_lock)

        new_params["temporary_lock"] = new_temporary_lock

    fractalLog(
        function="lockContainerAndUpdate",
        label=getContainerUser(container_name),
        logs="State: {state}, Lock: {lock}, Temporary Lock: {temporary_lock}".format(
            state=state, lock=str(lock), temporary_lock=str(temporary_lock),
        ),
    )

    container = UserContainer.query.filter_by(container_id=container_name)
    fractalSQLCommit(db, lambda _, x: x.update(new_params), container)


def checkLock(container_name):

    container = UserContainer.query.get(container_name)
    locked = False

    if container:
        locked = container.lock

    return locked


def spinLock(container_name, s=None):
    """Waits for container to be unlocked

    Args:
        container_name (str): Name of container of interest
        ID (int, optional): Unique papertrail logging id. Defaults to -1.

    Returns:
        int: 1 = container is unlocked, -1 = giving up
    """

    # Check if Container is currently locked

    container = UserContainer.query.get(container_name)

    username = None
    if container:
        username = container.user_id

    else:
        fractalLog(
            function="spinLock",
            label=str(username) if username else container_name,
            logs="spinLock could not find Container {container_name}".format(
                container_name=container_name
            ),
            level=logging.ERROR,
        )
        return -1

    locked = checkLock(container_name)
    num_tries = 0

    if not locked:
        fractalLog(
            function="spinLock",
            label=str(username),
            logs="Container {container_name} found unlocked on first try.".format(
                container_name=container_name
            ),
        )
        return 1
    else:
        fractalLog(
            function="spinLock",
            label=str(username),
            logs="Container {container_name} found locked on first try. Proceeding to wait...".format(
                container_name=container_name
            ),
        )
        if s:
            s.update_state(
                state="PENDING",
                meta={"msg": "Cloud PC is downloading an update. This could take a few minutes."},
            )

    while locked:
        time.sleep(5)
        locked = checkLock(container_name)
        num_tries += 1

        if num_tries > 40:
            fractalLog(
                function="spinLock",
                label=str(username),
                logs="Container {container_name} locked after waiting 200 seconds. Giving up...".format(
                    container_name=container_name
                ),
                level=logging.ERROR,
            )
            return -1

    if s:
        s.update_state(state="PENDING", meta={"msg": "Update successfully downloaded."})

    fractalLog(
        function="spinLock",
        label=str(username),
        logs="After waiting {seconds} seconds, Container {container_name} is unlocked".format(
            seconds=str(num_tries * 5), container_name=container_name
        ),
    )

    return 1
