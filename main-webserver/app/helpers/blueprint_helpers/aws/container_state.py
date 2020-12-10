from app.models import db, UserContainerState
from app.helpers.utils.general.sql_commands import fractal_sql_commit
from app.constants.container_state_values import CANCELLED, PENDING


def cancel_container(user):
    """Simply cancels the application for someone by changing the
    container state table.

    Args:
        user (str): The username of the user who's app we are cancelling
        (i.e. the spinup).
    """
    set_container_state(state=CANCELLED, keyuser=user)


def container_state_obj(**kwargs):
    """Fetches the UserContainerState object which has the user_id,
    state, and task_id (i.e. for the celery task, called statusID on the client app).
    This can be used to modify the table or read values.

    Returns:
        UserContainerState: An entry into the UserContainerState table
        (user_app_state in SQL).
    """
    user_id = kwargs.get("user_id", None)
    task_id = kwargs.get("task_id", None)

    _kwargs = {"user_id": user_id} if user_id else {"task_id": task_id}
    state_info = UserContainerState.query.filter_by(_kwargs).limit(1).first()
    return state_info


def can_update_container_state(user, task_id):
    """This let's us know if the user's entry in UserContainerState
    can be updated (meaning that the task_id which is calling this,
    passing its own task_id, is still the valid one for that
    entry: this is meant to avoid race conditions).

    This is used by the create_container endpoint to check whether it can set it
    to FAILURE or CANCELLED (imagine that the user cancels and initializes a new task,
    then this old task would not want to be able to overwrite the table.)

    Args:
        user (str): The username of the user who's entry we want to
        check for whether it can be updated.
        task_id (str): Thet task id (status id) of the celery task calling this.

    Returns:
        bool: Whether the task id matches and this hasn't been cancelled.
    """
    obj = container_state_obj(user_id=user)
    return obj.task_id == task_id and obj.state != CANCELLED


def set_container_state(**kwargs):
    """Sets the container state (or creates if necessary) entry
    for either a user or a task (filered by keyuser or keytask in **kwargs).
    It then modifies task_id, state, or user_id based on those **kwargs (by those names).

    It expects at least one of a user or task (as a key). If you are
    trying to create a new state, it expects both.

    Raises:
        KeyError: The user or task keys (keyuser, keytask) were never passed.
    """
    keyuser, keytask = kwargs.get("keyuser", None), kwargs.get("keytask", None)
    if not keyuser and not keytask:
        raise KeyError("Need either a user or a task")

    _kwargs = {"user_id": keyuser} if keyuser else {"task_id": keytask}
    obj = container_state_obj(**_kwargs)

    if obj:
        if "task_id" in kwargs:
            obj.task_id = kwargs["task_id"]
        if "state" in kwargs:
            obj.state = kwargs["state"]
        if "user_id" in kwargs:
            obj.user_id = kwargs["user_id"]
        db.session.commit()
    else:
        if "state" in kwargs:
            create_container_state(keyuser, keytask, state=kwargs["state"])
        else:
            create_container_state(keyuser, keytask)


def create_container_state(user_id, task_id, state=PENDING):
    """Creates a new entry into the app_info table.

    Args:
        user_id (str): The username of the user for whom'stdv this entry belongs.
        task_id (str): The task id of the task that's creating this.
        state (str, optional): The state that we want to write to the table for this
        new object.. Defaults to PENDING.

    Raises:
        Exception: if it fails to commit the creation somehow.
    """
    obj = UserContainerState(
        user_id=user_id,
        task_id=task_id,
        state=state,
    )
    sql = fractal_sql_commit(db, lambda db, x: db.session.add(x), obj)

    if not sql:
        raise Exception("Failed to commit new app state info.")
