import logging
import time

from app.helpers.utils.aws.aws_general import get_container_user
from app.helpers.utils.general.logs import fractal_log
from app.helpers.utils.general.sql_commands import fractal_sql_commit
from app.helpers.utils.general.time import (
    date_to_unix,
    get_today,
    shift_unix_by_minutes,
)
from app.models import db, UserContainer


def lock_container_and_update(container_name, state, lock, temporary_lock):
    """Change the state, lock, and temporary lock of a Container

    Args:
        container_name (str): Name of Container
        state (str): Desired state of Container
            [RUNNING_AVAILABLE, RUNNING_UNAVAILABLE, DEALLOCATED, DEALLOCATING, STOPPED, STOPPING,
            DELETING, CREATING, RESTARTING, STARTING]
        lock (bool): True if Container is locked, False otherwise
        temporary_lock (int): Number of minutes, starting from now, to lock the Container (max is
            10).

    Returns:
        int: 1 = container is unlocked, -1 = giving up
    """

    new_params = {"state": state, "lock": lock}

    if not state:
        new_params = {"lock": lock}

    if temporary_lock:
        new_temporary_lock = shift_unix_by_minutes(date_to_unix(get_today()), temporary_lock)

        new_params["temporary_lock"] = new_temporary_lock

    fractal_log(
        function="lock_container_and_update",
        label=get_container_user(container_name),
        logs="State: {state}, Lock: {lock}, Temporary Lock: {temporary_lock}".format(
            state=state,
            lock=str(lock),
            temporary_lock=str(temporary_lock),
        ),
    )

    container = UserContainer.query.filter_by(container_id=container_name)
    fractal_sql_commit(db, lambda _, x: x.update(new_params), container)


def lock_cluster_and_update(*args, **kwargs):
    """Acquire a lock on a row in the cluster_info table and update it.

    Arguments:
        TODO

    Returns:
        TODO
    """

    raise NotImplementedError


def check_lock(container_name):

    container = UserContainer.query.get(container_name)
    locked = False

    if container:
        locked = container.lock

    return locked


def spin_lock(container_name, state_obj=None):
    """Waits for container to be unlocked

    Args:
        container_name (str): Name of container of interest

    Returns:
        int: 1 = container is unlocked, -1 = giving up
    """

    # Check if Container is currently locked

    container = UserContainer.query.get(container_name)

    username = None
    if container:
        username = container.user_id

    else:
        fractal_log(
            function="spin_lock",
            label=str(username) if username else container_name,
            logs="spin_lock could not find Container {container_name}".format(
                container_name=container_name
            ),
            level=logging.ERROR,
        )
        return -1

    locked = check_lock(container_name)
    num_tries = 0

    if not locked:
        fractal_log(
            function="spin_lock",
            label=str(username),
            logs="Container {container_name} found unlocked on first try.".format(
                container_name=container_name
            ),
        )
        return 1
    else:
        fractal_log(
            function="spin_lock",
            label=str(username),
            logs=f"Container {container_name} found locked on first try. Proceeding to wait...",
        )
        if state_obj:
            state_obj.update_state(
                state="PENDING",
                meta={"msg": "Fractal is downloading an update. This could take a few minutes."},
            )

    while locked:
        time.sleep(5)
        locked = check_lock(container_name)
        num_tries += 1

        if num_tries > 40:
            fractal_log(
                function="spin_lock",
                label=str(username),
                logs=f"Container {container_name} locked after waiting 200 seconds. Giving up...",
                level=logging.ERROR,
            )
            return -1

    if state_obj:
        state_obj.update_state(state="PENDING", meta={"msg": "Update successfully downloaded."})

    fractal_log(
        function="spin_lock",
        label=str(username),
        logs="After waiting {seconds} seconds, Container {container_name} is unlocked".format(
            seconds=str(num_tries * 5), container_name=container_name
        ),
    )

    return 1
