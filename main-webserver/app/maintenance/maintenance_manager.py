"""Maintenance mode implementation.

This module implements the core functionality for webserver maintenance mode. We need a
maintenance mode because certain parts of our system are very susceptible to race
conditions during system updates. A system update involves new task definitions, new AMIs,
and critical database updates (in Region2AMI, for example) that are MUCH easier to
guarantee correctness when we know no one is creating new clusters/containers during
these updates. This file implements a distributed lock system using Redis that
tracks problematic tasks (create_cluster, assign_container). The webserver cannot be
put into maintenance mode while a problematic task is running. Also vice versa -
when the webserver is in maintenance mode, no problematic task can be run.

Any user of this module generally cares about the following functions:
- try_start_maintenance
- try_end_maintenance
- maintenance_track_task (decorator on celery functions that need to be tracked)
"""

import time
from functools import wraps
import inspect
import random
from typing import Callable, Tuple, Optional
import ssl

import redis
from celery import current_task

from app.helpers.utils.general.logs import fractal_logger


_REDIS_LOCK_KEY = "WEBSERVER_REDIS_LOCK"
_REDIS_MAINTENANCE_KEY = "WEBSERVER_MAINTENANCE_MODE"
_REDIS_TASKS_KEY = "WEBSERVER_TASKS"

_REDIS_CONN: redis.Redis


def maintenance_init_redis_conn(redis_uri: str):
    """
    Create a connection to redis if None already exists.

    Args:
        redis_uri: redis uri to connect to

    Returns:
        A redis connection
    """
    global _REDIS_CONN

    if redis_uri.startswith("rediss"):
        _REDIS_CONN = redis.from_url(
            redis_uri,
            db=1,  # this uses a different db than celery
            ssl_cert_reqs=ssl.CERT_NONE,
        )
    else:
        _REDIS_CONN = redis.from_url(
            redis_uri,
            db=1,  # this uses a different db than celery
        )


def check_if_maintenance() -> bool:
    """
    Check if webserver is in maintenance mode.

    Returns:
        True if and only if the web server is currently in maintenance mode.
    """
    # we don't need a lock because we are just checking if the key exists,
    # not modifying data. it is posisble for maintenance to start right after
    # the check happens; that is handled through task tracking and failing out
    # celery tasks that try to start during maintenance
    return bool(_REDIS_CONN.exists(_REDIS_MAINTENANCE_KEY))


def _get_lock(max_tries: int = 100, should_sleep: bool = True) -> bool:
    """
    Tries to get lock using redis SETNX command. Tries `max_tries` time with a
    sleep time chosen randomly between 0 and 1 seconds to improve lock contention.
    Expires the lock after 30 seconds.

    Args:
        max_tries: number of tries to get the lock
        should_sleep: True if method should sleep between attempts to grab lock

    Returns:
        True if and only if the lock is acquired.
    """
    for _ in range(max_tries):
        got_lock = _REDIS_CONN.setnx(_REDIS_LOCK_KEY, 1)

        if got_lock:
            # this key will last at most 30 sec.
            # we need this because webserver can be SIGKILL'd anytime, which with bad luck
            # could leave the lock "held" forever. anything holding the lock runs quickly,
            # so 30 seconds should be more than enough.
            _REDIS_CONN.expire(_REDIS_LOCK_KEY, 30)
            return True
        if should_sleep:
            sleep_time = random.random()
            time.sleep(sleep_time)
    return False


def _release_lock() -> None:
    """
    Release the lock
    """
    _REDIS_CONN.delete(_REDIS_LOCK_KEY)


def try_start_maintenance() -> Tuple[bool, str]:
    """
    Try to start maintenance mode. Steps:
        1. Grab the lock
        2. Set the maintenance key if no tracked tasks are running
        3. Release lock
    In order to prevent excessive polling of this method, _REDIS_MAINTENANCE_KEY
    will be created to prevent new tasks from being made. However, the
    webserver is not officially in maintenance mode until all problematic tasks finish.
    The first element of the return will only be True if all tasks have finished.
    However, this implementation allows a client to call this function once to stop new tasks
    from being started. Then, after some polling, the return should become true.

    Returns:
        (success, human_readable_msg)
        success: True if webserver in maintenance mode, otherwise false
        human_readable_msg: explains exactly what has happened
    """
    got_lock = _get_lock(10, should_sleep=False)  # only try 10 times since this runs synchronously
    if not got_lock:
        return False, "Did not get lock"

    # now we freely operate on the keys. we need to make sure no tasks are going
    success = False
    return_msg = None
    tasks = _REDIS_CONN.lrange(_REDIS_TASKS_KEY, 0, -1)
    if tasks is None or not tasks:
        # no tasks, we can start maintenance
        success = True
        # use an atomic setnx operation to start maintenance. The return is True if and only if this thread
        # created the maintenance key. This reduces log cluttering as only the thread that
        # starts maintenance ends up printing.
        maintenance_started = _REDIS_CONN.setnx(_REDIS_MAINTENANCE_KEY, 1)
        if maintenance_started:
            fractal_logger.info("Putting webserver into maintenance mode.")
            return_msg = "Put webserver into maintenance mode."
        else:
            return_msg = "Webserver is already in maintenance mode."

    else:
        # no tasks, we cannot start maintenance. However, we set maintenance key so no new tasks start.
        success = False
        _REDIS_CONN.set(_REDIS_MAINTENANCE_KEY, 1)

        log = (
            "Cannot start maintenance mode, but stopping new tasks from running."
            f" Waiting on {len(tasks)} task to finish. Task IDs: {tasks}."
        )
        fractal_logger.info(log)
        return_msg = (
            f"There are {len(tasks)} tasks currently running."
            f" However, all new maintenance tracked tasks will not run."
            " Please poll until these tasks finish."
        )

    _release_lock()
    return success, return_msg


def try_end_maintenance() -> Tuple[bool, str]:
    """
    Try to end maintenance mode. Steps:
        1. Grab the lock
        2. Delete the maintenance key
        3. Release lock

    Returns:
        True if and only if there is no maintenance mode. This can be because there already was no maintenance
        mode, or because it was ended by this function. This helps with github workflows
        because two workflows might both end maintenance. Until we solve our concurrent
        workflows problem, it's best to implement it this way.
    """
    got_lock = _get_lock(10, should_sleep=False)  # only try 10 times since this runs synchronously
    if not got_lock:
        return False, "Did not get lock."

    # now we freely operate on the keys. we simply delete the maintenance key. the return is False if the
    # key did not exist, and True if the key did exist and was deleted.
    ended_maintenance = _REDIS_CONN.delete(_REDIS_MAINTENANCE_KEY)
    if not ended_maintenance:
        _release_lock()
        # see function docstring for why we return True here
        return True, "Webserver was not in maintenance mode."

    fractal_logger.info("Ended webserver maintenance mode.")

    _REDIS_CONN.delete(_REDIS_MAINTENANCE_KEY)
    _release_lock()

    return True, "Ended webserver maintenance mode."


def try_register_task(task_id: int) -> bool:
    """
    Try to register a tracked task. Steps:
        1. Grab the lock
        2. Check if maintenance mode is going on; if so error out
        3. Add task_id to the tasks list
        3. Release lock

    Args:
        task_id: celery task that is being tracked

    Returns:
        True if registered, False if lock was not acquired or maintenance is happening
    """
    got_lock = _get_lock(100)
    if not got_lock:
        return False

    success = False
    # now we freely operate on the keys. first check if an maintenance is in progress. if so,
    # the task does not get registered. otherwise, register it.
    maintenance_exists = _REDIS_CONN.exists(_REDIS_MAINTENANCE_KEY)
    if maintenance_exists:
        success = False
    else:
        _REDIS_CONN.rpush(_REDIS_TASKS_KEY, task_id)
        # this key will last at most 900 sec = 15 minutes before being deleted
        # we need this because tasks have no guarantee of termination; this
        # stops us from tracking outdated tasks.
        # any existing expire time on a key is reset after this invocation.
        _REDIS_CONN.expire(_REDIS_TASKS_KEY, 900)
        success = True

    _release_lock()
    return success


def try_deregister_task(task_id: int) -> bool:
    """
    Try to deregister a tracked task. Steps:
        1. Grab the lock
        2. Remove task_id from the tasks list
        3. Release lock

    Args:
        task_id: celery task that is being tracked

    Returns:
        True if deregistered, False if lock was not acquired or task_id does not exist in list
    """
    got_lock = _get_lock(100)
    if not got_lock:
        return False

    # remove from list
    success = _REDIS_CONN.lrem(_REDIS_TASKS_KEY, 1, task_id) == 1
    _release_lock()
    return success


def maintenance_track_task(func: Callable) -> Callable:
    """
    Decorator to do three things:
        1. check if there is no maintenance going on. if so error out and never call the celery task.
        2. track a celery task during its execution
        3. clean up tracking of the celery task after it finishes
    """

    @wraps(func)
    def wrapper(*args, **kwargs):

        # pre-function logic: register the task
        if not try_register_task(current_task.request.id):
            msg = (
                "Could not register a tracked task. "
                f"Celery Task ID: {current_task.request.id}. "
                f"Function: {func.__name__}. "
                f"Args: {args}. "
                f"Kwargs: {kwargs}."
            )
            fractal_logger.info(msg)
            raise ValueError("MAINTENANCE ERROR. Failed to register task.")

        fractal_logger.info(f"Registered tracked task: {current_task.request.id}.")

        exception = None
        try:
            ret = func(*args, **kwargs)
        except Exception as e:  # pylint: disable=broad-except
            exception = e

        # post-function logic: deregister the task
        if not try_deregister_task(current_task.request.id):
            msg = (
                f"CRITICAL! Could not deregister a tracked task. "
                f"Celery Task ID: {current_task.request.id}. "
                f"Function: {func.__name__}. "
                f"Args: {args}. "
                f"Kwargs: {kwargs}."
            )
            fractal_logger.error(msg)
            # exception can be None here
            raise ValueError("MAINTENANCE ERROR. Failed to deregister task.") from exception

        fractal_logger.info(f"Deregistered tracked task: {current_task.request.id}.")

        # raise any exception to caller
        if exception is not None:
            raise exception
        return ret

    return wrapper
