from app.models import db, UserContainerState
from app.helpers.utils.general.sql_commands import fractal_sql_commit
from app.constants.container_state_values import CANCELLED, PENDING


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
    state_info = UserContainerState.query.filter_by(**_kwargs).limit(1).first()
    return state_info


def can_update_container_state(user, task_id, obj=None):
    """This let's us know if the user's entry in UserContainerState
    can be updated (meaning that the task_id which is calling this,
    passing its own task_id, is still the valid one for that
    entry: this is meant to avoid race conditions).

    This is used by the create_container endpoint to check whether it can set it
    to FAILURE or CANCELLED (imagine that the user cancels and initializes a new task,
    then this old task would not want to be able to overwrite the table.)

    If obj is null then it will pass since that signifies creation. If task_id is null
    then it will also pass signifying that we do not actually care about the task_id.
    However, if this is used as a prelude to creation of an entry, that
    creation will fail since the column is non-nullable.

    Args:
        user (str): The username of the user who's entry we want to
        check for whether it can be updated.
        task_id (str): Thet task id (status id) of the celery task calling this.
        obj (UserContainerState) : an object that can be passed if it's being used inside
        an internal method (such as set_container_state without force). Defaults to null.

    Returns:
        bool: Whether the task id matches and this hasn't been cancelled.
    """
    if not obj:
        obj = container_state_obj(user_id=user)
    return (not obj or not task_id) or (obj.task_id == task_id and obj.state != CANCELLED)


def set_container_state(keyuser, keytask, user_id=None, state=None, task_id=None, force=False):
    """Set a container state in the UserContinerState (user_app_state) table. We
    require a keyuser (and potentially keytask) to set it. You can null out keytask only if
    the entry already exists and force is true. Otherwise keytask is required to check
    whether that entry can be updated. It can be updated only if the keytask matches the
    current task and it's not cancelled. The other params (aside from force) set those
    values in the entry in the table.

    Args:
        keyuser (str): The user to filter by.
        keytask (str): The task id that we should expect to be the current task
        in that entry.
        user_id (str, optional): The user_id we want to set. Defaults to None.
        state (str, optional): The state we want to set. Defaults to None.
        task_id (str, optional): The task id we want to set. Defaults to None.
        force (bool, optional): Whether to update with a check for validity or not.
        Defaults to False.
    """
    obj = container_state_obj(user_id=keyuser)

    if force or can_update_container_state(keyuser, keytask, obj=obj):
        if obj:
            if task_id:
                obj.task_id = task_id
            if state:
                obj.state = state
            if user_id:
                obj.user_id = user_id
            db.session.commit()
        else:
            if not state:
                state = PENDING
            create_container_state(keyuser, keytask, state=state)


def create_container_state(user_id, task_id, state=PENDING):
    """Creates a new entry into the app_info table.

    Args:
        user_id (str): The username of the user for whom'stdv this entry belongs.
        task_id (str): The task id of the task that's creating this.
        state (str, optional): The state that we want to write to the table for this
        new object. Defaults to PENDING.

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
