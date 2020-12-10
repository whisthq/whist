from app.models.hardware import db, UserAppState
from app.helpers.utils.general.sql_commands import fractal_sql_commit
from app.constants.user_app_states import CANCELLED, PENDING

# note that this user should be unique


def cancel_app(user):
    set_app_state(CANCELLED, user_id=user)


def _get_app_info_obj(**kwargs):
    user_id = kwargs.get("user_id", None)
    task_id = kwargs.get("task_id", None)

    _kwargs = {"user_id": user_id} if user_id else {"task_id": task_id}
    state_info = UserAppState.query.filter_by(_kwargs).limit(1).first()
    return state_info


# let us know if the filter by user matches task_id
def can_update_app_state(user, task_id):
    obj = _get_app_info_obj(user_id=user)
    return obj.task_id == task_id and obj.state != CANCELLED


# expects a user or task as a key and kwargs for what to set things to
# those should be user_id etc...
# this will initialize automatically
def set_app_info(**kwargs):
    keyuser, keytask = kwargs.get("keyuser", None), kwargs.get("keytask", None)
    if not keyuser and not keytask:
        raise KeyError("Need either a user or a task")

    _kwargs = {"user_id": keyuser} if user else {"task_id": keytask}
    obj = _get_app_info_obj(_kwargs)

    if obj:
        if "task_id" in kwargs:
            obj.task_id = kwargs["task_id"]
        if "state" in kwargs:
            obj.state = kwargs["state"]
        if "user_id" in kwargs:
            obj.user_id = kwargs["user_id"]
        db.session.commit()
    else:
        create_app_state_info(
            keyuser, task_id, kwargs["state"]
        ) if "state" in kwargs else create_app_state_info(keyuser, task_id)


def get_app_info(**kwargs):
    obj = _get_app_info_obj(kwargs)
    return state_info.user_id, state_info.state, state_info.task_id


def create_app_info(user_id, task_id, state=PENDING):
    state_info = UserAppState(
        user_id=user_id,
        task_id=task_id,
        state=state,
    )
    sql = fractal_sql_commit(db, lambda db, x: db.session.add(x), state_info)

    if not sql:
        raise Exception("Failed to commit new app state info.")
