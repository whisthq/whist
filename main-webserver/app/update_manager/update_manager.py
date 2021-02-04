import time
from functools import wraps
import json
import inspect
from typing import Callable

import redis

from app.helpers.utils.general.logs import fractal_log
from app.helpers.utils.slack.slack import slack_send_safe


_REDIS_LOCK_KEY = "WEBSERVER_REDIS_LOCK"
_REDIS_UPDATE_KEY = "WEBSERVER_UPDATE_{region_name}"
_REDIS_TASKS_KEY = "WEBSERVER_TASKS_{region_name}"


def try_start_update(region_name: str) -> bool:
    from app import redis_conn

    update_key = _REDIS_UPDATE_KEY.format(region_name=region_name)
    tasks_key = _REDIS_TASKS_KEY.format(region_name=region_name)

    success = False
    got_lock = redis_conn.setnx(_REDIS_LOCK_KEY, 1)
    if not got_lock:
        return False

    # now we freely operate on the keys. first make sure we are not already in update mode
    # then, we need to make sure no tasks are going
    update_exists = redis_conn.exists(update_key)
    if update_exists:
        # release lock
        redis_conn.delete(_REDIS_LOCK_KEY)
        return False

    tasks = redis_conn.lrange(tasks_key, 0, -1)
    if tasks is None or len(tasks) == 0:
        # no tasks, we can start the update
        success = True
        redis_conn.set(update_key, 1)
        fractal_log(
            "try_start_update", None, f"putting webserver into update mode on region {region_name}"
        )
        slack_send_safe(
            "#alerts-test", f":lock: webserver is in maintenance mode on region {region_name}"
        )

    else:
        fractal_log(
            "try_start_update",
            None,
            f"cannot start update. waiting on {len(tasks)} task to finish. Task IDs:\n{tasks}.",
        )

    # release lock
    redis_conn.delete(_REDIS_LOCK_KEY)
    return success


def try_end_update(region_name: str) -> bool:
    from app import redis_conn

    update_key = _REDIS_UPDATE_KEY.format(region_name=region_name)
    got_lock = redis_conn.setnx(_REDIS_LOCK_KEY, 1)
    if not got_lock:
        return False

    #  now we freely operate on the keys. first make sure we are not already in update mode
    # then, we need to make sure no tasks are going
    update_exists = redis_conn.exists(update_key)
    if not update_exists:
        # release lock
        redis_conn.delete(_REDIS_LOCK_KEY)
        return False

    fractal_log("try_end_update", None, f"ending webserver update mode on region {region_name}")
    # delete update key
    redis_conn.delete(update_key)
    # release lock
    redis_conn.delete(_REDIS_LOCK_KEY)
    # notify slack
    slack_send_safe(
        "#alerts-test", f":unlock: webserver has ended maintenance mode on region {region_name}"
    )
    return True


def try_register_task(region_name: str, task_id: int) -> bool:
    from app import redis_conn

    update_key = _REDIS_UPDATE_KEY.format(region_name=region_name)
    tasks_key = _REDIS_TASKS_KEY.format(region_name=region_name)

    got_lock = redis_conn.setnx(_REDIS_LOCK_KEY, 1)
    if not got_lock:
        return False

    success = False

    # now we freely operate on the keys. first check if an update is in progress. if so,
    # the task does not get registered. otherwise, register it.
    update_exists = redis_conn.exists(update_key)
    if update_exists:
        success = False
    else:
        redis_conn.rpush(tasks_key, task_id)
        success = True

    # release lock
    redis_conn.delete(_REDIS_LOCK_KEY)
    return success


def try_deregister_task(region_name: str, task_id: int) -> bool:
    from app import redis_conn

    tasks_key = _REDIS_TASKS_KEY.format(region_name=region_name)

    got_lock = redis_conn.setnx(_REDIS_LOCK_KEY, 1)
    if not got_lock:
        return False

    # remove from list
    success = redis_conn.lrem(tasks_key, 1, task_id)

    # release lock
    redis_conn.delete(_REDIS_LOCK_KEY)
    return success


def wait_register_task(region_name: str, task_id: str, max_tries: int) -> bool:
    for i in range(max_tries):
        fractal_log("wait_register_task", None, f"Try {i}: registering task {task_id}...")
        if try_register_task(region_name, task_id):
            return True
        time.sleep(10)
    return False


def wait_degister_task(region_name: str, task_id: str, max_tries: int) -> bool:
    for i in range(max_tries):
        fractal_log("wait_degister_task", None, f"Try {i}: deregistering task {task_id}...")
        if try_deregister_task(region_name, task_id):
            return True
        time.sleep(10)
    return False


def get_arg_number(func: Callable, desired_arg: str) -> int:
    """
    Given a function `func`, this function gives the argument number of `desired_arg`
    in `func`'s arguments.

    Args:
        func (Callable): a python function that takes arguments
        desired_arg (str): the desired argument
    """
    args = inspect.getfullargspec(func).args
    if args is None:
        return -1
    for argn, arg in enumerate(args):
        if desired_arg == arg:
            return argn
    return -1


def wait_no_update_and_track_task(func):
    # must have "self" argument
    self_argn = get_arg_number(func, "self")
    if self_argn == -1:
        raise ValueError(f"Function {func.__name__} needs to have argument self to be tracked")

    # must have "region" or "region_name" argument
    region_argn = get_arg_number(func, "region")
    if region_argn == -1:
        region_argn = get_arg_number(func, "region_name")
        if region_argn == -1:
            raise ValueError(f"Function {func.__name__} needs to have argument self to be tracked")

    @wraps(func)
    def wrapper(*args, **kwargs):
        self_obj = args[self_argn]
        region_name = args[region_argn]

        # pre-function logic: register the task
        if not wait_register_task(region_name, self_obj.request.id, 10):
            raise ValueError(f"failed to register task. function: {func.__name__}")

        exception = None
        try:
            ret = func(*args, **kwargs)
        except Exception as e:
            exception = e

        # post-function logic: deregister the task
        if not wait_degister_task(region_name, self_obj.request.id, 10):
            raise ValueError(f"failed to register task. function: {func.__name__}")

        # raise any exception to caller
        if exception is not None:
            raise exception
        return ret

    return wrapper
