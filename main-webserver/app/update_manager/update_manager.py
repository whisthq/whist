import logging
import time
from functools import wraps
import json
import inspect
import random
from typing import Callable, Tuple, Any

from flask import jsonify
import redis

from app.helpers.utils.general.logs import fractal_log
from app.helpers.utils.slack.slack import slack_send_safe
from app.constants.http_codes import WEBSERVER_MAINTENANCE


_REDIS_LOCK_KEY = "WEBSERVER_REDIS_LOCK"
_REDIS_UPDATE_KEY = "WEBSERVER_UPDATE_{region_name}"
_REDIS_TASKS_KEY = "WEBSERVER_TASKS_{region_name}"


def check_if_update(region_name: str) -> bool:
    """
    Check if an update is going on.

    Args:
        region_name: name of region to check for updates
    """
    from app import redis_conn

    update_key = _REDIS_UPDATE_KEY.format(region_name=region_name)
    # we don't need a lock because we are just checking if the key exists,
    # not modifying data. it is posisble for an update to start right after
    # the check happens; that is handled elsewhere.
    return redis_conn.exits(update_key)


def try_start_update(region_name: str) -> Tuple[bool, str]:
    """
    Try to start an update. Steps:
        1. Grab the lock
        2. Set the update key if no tracked tasks are running
        3. Release lock
    In order to prevent excessive polling of this method, the region's update
    key will be created to prevent new tasks from being made. However, the 
    webserver is not officially in update mode until all tasks in the region finish.
    The first element of the return will only be True if all tasks have finished.
    However, the implementation allows a client to call this function once to stop new tasks 
    from being started. The client just needs to grab the lock once to do so. 
    Then, after some polling, the return should become true. Also, there are 
    slack integrations to inform us what is going on.

    Args:
        region_name: name of region to start updating

    Return:
        (success, human_readable_msg)
        success: True if webserver in update mode, otherwise false
        human_readable_msg: explains exactly what has happened
    """
    from app import redis_conn

    update_key = _REDIS_UPDATE_KEY.format(region_name=region_name)
    tasks_key = _REDIS_TASKS_KEY.format(region_name=region_name)

    got_lock = redis_conn.setnx(_REDIS_LOCK_KEY, 1)
    if not got_lock:
        return False, "Did not get lock"

    # now we freely operate on the keys. first make sure we are not already in update mode
    # then, we need to make sure no tasks are going
    update_exists = redis_conn.exists(update_key)
    if update_exists:
        # release lock
        redis_conn.delete(_REDIS_LOCK_KEY)
        return False, "An update is already happening."

    success = False
    return_msg = None
    tasks = redis_conn.lrange(tasks_key, 0, -1)
    if tasks is None or len(tasks) == 0:
        # no tasks, we can start the update
        success = True
        redis_conn.set(update_key, 1)

        slack_send_safe(
            #TODO: make real
            "#alerts-test", 
            f":lock: webserver is in maintenance mode in region {region_name}"
        )
        fractal_log(
            "try_start_update",
            None,
            f"putting webserver into update mode in region {region_name}"
        )
        return_msg = f"Put webserver into maintenance mode in region {region_name}."

    else:
        # no tasks, we cannot start the update. However, we set update_key so no new tasks start.
        success = False
        redis_conn.set(update_key, 1)

        slack_msg = (
            f":no_entry_sign: webserver has stopped new tasks in region {region_name}."
            f"Waiting on {len(tasks)} to finish. Task IDs:\n{tasks}."
        )
        slack_send_safe(
            #TODO: make real
            "#alerts-test",
            slack_msg,
        )
        log = (
            "cannot start update, but stopping new tasks from running."
            f"waiting on {len(tasks)} task to finish. Task IDs:\n{tasks}."
        )
        fractal_log(
            "try_start_update",
            None,
            log,
        )
        return_msg = (
            f"There are {len(tasks)} tasks currently running."
            "However, all new tasks in region {region_name} will not run."
            "Please poll until these tasks finish."
        )

    # release lock
    redis_conn.delete(_REDIS_LOCK_KEY)
    return success, return_msg


def try_end_update(region_name: str) -> bool:
    """
    Try to end an update. Steps:
        1. Grab the lock
        2. Delete the update key
        3. Release lock
    Pings slack if the operation succeeds.

    Args:
        region_name: name of region to end updating

    Return:
        True if update ended, False if lock was not acquired or an update was not happening
    """
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

    fractal_log(
        "try_end_update",
        None,
        f"ending webserver update mode on region {region_name}",
    )
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
    """
    Try to register a tracked task. Steps:
        1. Grab the lock
        2. Check if an update is going on; if so return false
        3. Add task_id to the tasks list
        3. Release lock

    Args:
        region_name: name of region to start updating
        task_id: celery task that is being tracked

    Return:
        True if registered, False if lock was not acquired or an update is happening
    """
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
    """
    Try to deregister a tracked task. Steps:
        1. Grab the lock
        2. Remove task_id from the tasks list
        3. Release lock

    Args:
        region_name: name of region to start updating
        task_id: celery task that is being tracked

    Return:
        True if deregistered, False if lock was not acquired or task_id does not exist in list
    """
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


def wait_retry_func(func: Callable, max_tries: int, *args, **kwargs) -> bool:
    """
    Wrapper around any callable `func`. Tries max_tries times with sleep for a random
    amount of seconds between 0 and 3 (to improve lock contention).

    Args:
        task_id: celery task that is being tracked
        max_tries: times to try
        *args: any args for the function
        **kwargs: any kwargs for the function

    Return:
        True if func succeeded once, otherwise False.
    """
    for i in range(max_tries):
        fractal_log(
            "wait_retry_func",
            None,
            f"Try {i}: Calling {func.__name__} with args: {args}, kwargs: {kwargs}.",
        )
        if func(*args, **kwargs):
            return True
        sleep_time = random.random() * 3
        time.sleep(sleep_time)
    return False


def get_arg_number(func: Callable, desired_arg: str) -> int:
    """
    Given a function `func`, this function gives the argument number of `desired_arg`
    in `func`'s arguments.

    Args:
        func (Callable): a python function that takes arguments
        desired_arg (str): the desired argument

    Returns:
        index in *args of desired_arg, -1 if not found.
    """
    args = inspect.getfullargspec(func).args
    if args is None:
        return -1
    for argn, arg in enumerate(args):
        if desired_arg == arg:
            return argn
    return -1


def wait_no_update_and_track_task(func: Callable):
    """
    Decorator to do three things:
        1. check if there is no update going on. if so error out and never call the celery task.
        2. track a celery task during its execution
        3. clean up tracking of the celery task after it finishes

    Args:
        func: function (celery task) to decorate
    """
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
        self_obj = args[self_argn] # celery "self" object created with @shared_task(bind=True)
        region_name = args[region_argn]

        # pre-function logic: register the task
        if not wait_retry_func(
            try_register_task,
            10,
            # these are passed to try_register_task
            region_name,
            self_obj.request.id,
            ):
            raise ValueError(f"failed to register task. function: {func.__name__}")

        exception = None
        try:
            ret = func(*args, **kwargs)
        except Exception as e:
            exception = e

        # post-function logic: deregister the task
        if not wait_retry_func(
            try_deregister_task,
            10,
            # these are passed to try_deregister_task
            region_name,
            self_obj.request.id,
            ):
            raise ValueError(f"failed to deregister task. function: {func.__name__}")

        # raise any exception to caller
        if exception is not None:
            raise exception
        return ret

    return wrapper


def check_if_update_mode(func: Callable):
    """
    Decorator to do two things:
        1. check if there is no update going on. if so error out and never call the celery task.
        2. track a celery task during its execution
        3. clean up tracking of the celery task after it finishes

    Args:
        func: function (celery task) to decorate

    Return:
        If not in update mode, returns whatever `func` would. If in update mode, returns
        an error with WEBSERVER_MAINTENANCE code and never calls `func`. Special case:
        `func` == "test_endpoint" and argument "action" is "update_region" or "end_update".
        These are allowed during an update so we let them through.
    """

    @wraps(func)
    def wrapper(*args, **kwargs):
        body = kwargs.pop("body")
        region_name = None
        if "region" in kwargs:
            region_name = body["region"]
        elif "region_name" in kwargs:
            region_name = body["region_name"]
        else:
            # this shouldn't happen because this decorator should only work on function that
            # take region or region_name as paramaters in their requests.
            # Behavior: call the function to be safe but error log so we investigate
            log = (
                "Could not find region or region_name in kwargs for endpoint function"
                f"{func.__name__}. Continuing execution..."
            )
            fractal_log(
                "check_if_update_mode",
                None,
                log,
                level=logging.ERROR,
            )
            return func()

        # this is a special case: /aws_container/test_endpoint/update_region and 
        # /aws_container/test_endpoint/end_update can always proceed in maintenance mode
        if func.__name__ == 'test_endpoint':
            # TODO: can we do this differently? this is very jank
            action = args[0]
            if action in ["update_region", "end_update"]:
                return func()

        # check if in maintenance mode
        if check_if_update(region_name):
            log = (
                f"Could not service request because in maintenance mode. Function: {func.__name__}"
                f" Args: {*args}. Kwargs: {**kwargs}"
            )
            fractal_log(
                "check_if_update_mode",
                None,
                log,
            )

            return (
                jsonify(
                    {
                        "error": "Webserver is in maintenance mode.",
                    }
                ),
                WEBSERVER_MAINTENANCE,
            )
        else:
            # we are not in update mode; freely call the function
            return func()

    return wrapper
