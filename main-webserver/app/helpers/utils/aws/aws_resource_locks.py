import time

from app.helpers.utils.aws.aws_general import get_container_user
from app.helpers.utils.general.logs import fractal_logger
from app.models import db, UserContainer


def lock_container_and_update(container_name, state, wait=0):
    """Change the state, lock, and temporary lock of a Container

    Args:
        container_name (str): Name of Container
        state (str): Desired state of Container
            [RUNNING_AVAILABLE, RUNNING_UNAVAILABLE, DEALLOCATED, DEALLOCATING, STOPPED, STOPPING,
            DELETING, CREATING, RESTARTING, STARTING]
        wait (int): delay between acquiring lock and committing the update (for testing)

    Returns:
        int: 1 = container is unlocked, -1 = giving up
    """

    fractal_logger.info(
        "State: {state}".format(
            state=state,
        ),
        extra={"label": get_container_user(container_name)},
    )
    db.session.execute("SET LOCAL lock_timeout='30s';")
    # lock using with_for_update()
    container = UserContainer.query.with_for_update().filter_by(container_id=container_name).first()

    # optional wait time for testing
    time.sleep(wait)

    # update state and commit
    container.state = state
    db.session.commit()


def lock_cluster_and_update(*args, **kwargs):
    """Acquire a lock on a row in the cluster_info table and update it.

    Arguments:
        TODO

    Returns:
        TODO
    """

    raise NotImplementedError
