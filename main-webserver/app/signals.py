"""
This modules contains handlers for the following signals:
1. SIGTERM. Sent by Heroku before dyno restart. See
https://www.notion.so/tryfractal/Resolving-Heroku-Dyno-Restart-db63f4cbb9bd49a1a1fdab7aeb1f77e6
for more details on when this happens and how we are solving it.
"""
import signal
import threading
from typing import List

from celery.signals import worker_shutting_down, task_received, task_postrun
from celery import current_app
from celery.worker.request import Request
import gevent

from app.helpers.utils.general.logs import fractal_logger
from app import set_web_requests_status

_TASKS_SET = set()
_TASKS_LOCK = threading.Lock()


class WebSignalHandler:
    """
    Handles signals for the web dyno. Specifically:
    - SIGTERM (`handle_sigterm`)

    Initialize in webserver with:
    >>> WebSignalHandler() # signals are now handled
    """

    def __init__(self):
        signal.signal(signal.SIGTERM, self.handle_sigterm)

    def handle_sigterm(self, signum, frame):  # pylint: disable=no-self-use,unused-argument
        """
        Handles SIGTERM, which is sent by Heroku on various conditions outlined here:
        https://www.notion.so/tryfractal/Resolving-Heroku-Dyno-Restart-db63f4cbb9bd49a1a1fdab7aeb1f77e6

        SIGKILL follows 30 seconds from now, so this is our chance to clean-up resources cleanly.
        Specifically, we do the following:
        1. Flip a flag to stop incoming webserver requests, thereby making sure a webserver request
        is not interrupted. We don't kill webserver now because a web request might be ongoing.
        """
        # 1. disallow web requests
        if not set_web_requests_status(False):
            fractal_logger.error("Could not disable web requests after SIGTERM.")


@task_received.connect
def handle_task_received(request: Request, **kwargs):  # pylint: disable=unused-argument
    """
    See https://docs.celeryproject.org/en/stable/userguide/signals.html#task-received.
    Runs after a task is picked up by this worker.

    Args:
        request: contains info on task. Provided by celery.
        kwargs: required by celery.
    """
    lock_acquired = _TASKS_LOCK.acquire(blocking=True, timeout=10)
    if not lock_acquired:
        fractal_logger.error(f"Failed to acquire tasks lock for task {request.id}")
        return
    try:
        _TASKS_SET.add(request.id)
    except:
        fractal_logger.error(f"Could not add {request.id} to _TASKS_SET.", exc_info=True)
    finally:
        _TASKS_LOCK.release()


@task_postrun.connect
def handle_task_postrun(
    task_id: str, task, retval: int, state: str, **kwargs
):  # pylint: disable=unused-argument
    """
    See https://docs.celeryproject.org/en/stable/userguide/signals.html#task_postrun.
    Runs after a task is finished by this worker.

    Args:
        task_id: Task ID that finished.
        task: a tasks.inner object from celery
        retval: return value of the task
        state: return state of the task
        kwargs: required by celery.
    """
    lock_acquired = _TASKS_LOCK.acquire(blocking=True, timeout=10)
    if not lock_acquired:
        fractal_logger.error(f"Failed to acquire tasks lock for task {task_id}")
        return
    try:
        _TASKS_SET.remove(task_id)
    except:
        fractal_logger.error(f"Could not remove {task_id} from _TASKS_SET.", exc_info=True)
    finally:
        _TASKS_LOCK.release()


@worker_shutting_down.connect
def celery_signal_handler(sig, how, exitcode, **kwargs):  # pylint: disable=unused-argument
    """
    See https://docs.celeryproject.org/en/stable/userguide/signals.html#worker_shutting_down.
    Runs after a signal occurs and celery runs its default handlers.

    Args:
        sig: string indicating type of signal. Ex: SIGTERM, SIGINT
        how: "Warm" or "Cold". SIGTERM and SIGINT have two layers of handlers. The first
            occurrence starts a "Warm" shutdown that stops new tasks from being processed
            but waits for existing ones to finish. The second occurence is a "Cold" shutdown
            that just kills the worker.
        exitcode: integer return.
        kwargs: required by celery.
    """
    if sig != "SIGTERM":
        return

    fractal_logger.info("Running SIGTERM handler")
    # we don't need the lock because this runs in signal (syscall->userspace) context
    # and celery with gevent is single-core, so the main program is not running
    all_tasks = list(_TASKS_SET)
    if len(all_tasks) > 0:
        # we cannot do the work in the signal handler because it is blocking (redis operations)
        # gevent has monkeypatched all blocking libraries, so it would try to perform
        # a userspace context switch. we should not do that in a signal handler.
        # instead we give gevent a task to run later.
        gevent.Greenlet.spawn(mark_tasks_as_revoked, all_tasks)


def mark_tasks_as_revoked(task_ids: List[str]):
    """
    Mark non-finished tasks in `task_ids` as REVOKED in backend. Ideally, we could call
    `app.control.revoke(task_ids)` here, but because this runs after celery has
    run its own SIGTERM handler (which stops the worker from picking up new tasks
    and essentially takes it offline) the worker never sees the revoke command.
    Even if it did, https://github.com/celery/celery/issues/4019 indicates that
    revoking tasks with gevent is not possible. With some hacking we might be
    able to solve this, but remember we have 30 seconds to clean up state. The
    following races might happen:

    1.
        - Task succeeds
        - This function runs and marks it as revoked despite success
        We handle this by checking to see if the task status is one of PENDING
        or STARTED. Only in that case do we overwrite the result.

    2.
        - This function runs and marks a task as revoked
        - Task succeeds and overrides the revoked status (in the 30 seconds it has)
        There is not much we can do about this without much more work. Also this is
        an acceptable outcome because the task properly finished.

    Args:
        task_ids: List of task ids to revoke
    """
    for task_id in task_ids:
        task_result = current_app.AsyncResult(task_id)
        if task_result.state in ("PENDING", "STARTED"):
            # either of these states means the task did not start or did not finish
            # so we mark it as revoked.
            current_app.backend.store_result(task_id, None, "REVOKED")
    fractal_logger.info(f"Marked the following tasks as revoked: {task_ids}")
