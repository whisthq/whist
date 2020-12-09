from app.models.hardware import db, UserAppState
from app.helpers.utils.general.sql_commands import fractal_sql_commit
from app.constants.user_app_states import CANCELLED, PENDING

# note that this user should be unique


def cancel_app(user):
    set_app_state(CANCELLED, user_id=user)


def _get_app_info_obj(**kwargs):
    user_id = None
    task_id = None
    for user in ["user", "email", "user_id", "username"]:
        if user in kwargs:
            user_id = kwargs[user]
    for task in ["task", "task_id", "celery_task_id"]:
        if task in kwargs:
            task_id = kwargs[task_id]
    _kwargs = {"user_id": user_id} if user_id else {"task_id": task_id}
    state_info = UserAppState.query.filter_by(_kwargs).limit(1).first()
    return state_info


def get_app_info(**kwargs):
    obj = _get_app_info_obj(kwargs)
    return state_info.user_id, state_info.state, state_info.task_id


def set_app_state(state, **kwargs):
    obj = _get_app_info_obj(kwargs)
    obj.state = state
    db.session.commit()


def set_app_task_id(task_id, **kwargs):
    obj = _get_app_info_obj(kwargs)
    obj.task_id = task_id
    db.session.commit()


def create_app_state_info(user_id, task_id, state=PENDING):
    state_info = UserAppState(
        user_id=user_id,
        task_id=task_id,
        state=state,
    )
    sql = fractal_sql_commit(db, lambda db, x: db.session.add(x), state_info)

    if not sql:
        raise Exception("Failed to commit new app state info.")
