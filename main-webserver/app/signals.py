"""
This modules contains handlers for the following signals:
1. SIGTERM. Sent by Heroku before dyno restart. See
https://www.notion.so/tryfractal/Resolving-Heroku-Dyno-Restart-db63f4cbb9bd49a1a1fdab7aeb1f77e6
for more details on when this happens and how we are solving it.

It also installs the following celery handlers (for use on web and celery workers):
- before_task_publish
"""
import signal
from typing import List

from celery.signals import before_task_publish, worker_shutting_down
from celery import current_app
import gevent

from app.helpers.utils.general.logs import fractal_logger
from app.flask_handlers import set_web_requests_status


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


@before_task_publish.connect
def celery_before_task_publish_handler(
    body,
    exchange,
    routing_key,
    headers,
    properties,
    declare,
    retry_policy,
    **kwargs,  # pylint: disable=unused-argument
):
    """
    See https://docs.celeryproject.org/en/stable/userguide/signals.html#before-task-publish
    for an explanation on the arguments and the before_task_publish celery signal.
    Runs directly after a celery task is declared, but before it is published. This will
    run before .delay(...) returns. We use this to explicitly mark tasks as PENDING in the
    backend. See https://github.com/fractal/fractal/pull/1556 for a full explanation.
    """
    task_id = headers["id"]
    current_app.backend.store_result(task_id, None, "PENDING")


@worker_shutting_down.connect
def celery_signal_handler(sig, how, exitcode, **kwargs):  # pylint: disable=unused-argument
    """
    See https://docs.celeryproject.org/en/stable/userguide/signals.html#worker_shutting_down.
    Runs after a signal occurs and celery runs its default handlers. This signal handler
    only works with the gevent pooling option. For unknown reasons, `celery.worker.state`
    has no requests is empty if we use prefork. Our tests make sure this works, so
    we can have confidence this won't break silently.

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
    # this must be imported here as it will be defined after startup
    from celery.worker import state

    # state.active_requests is a set of celery.worker.request.Request objects
    worker_task_ids = [req.id for req in state.active_requests]
    if len(worker_task_ids) > 0:
        # we cannot do the work in the signal handler because it is blocking (redis operations)
        # gevent has monkeypatched all blocking libraries, so it would try to perform
        # a userspace context switch. we should not do that in a signal handler.
        # instead we give gevent a task to run later.
        gevent.Greenlet.spawn(mark_tasks_as_revoked, worker_task_ids)


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
    revoked_task_ids = []
    for task_id in task_ids:
        task_result = current_app.AsyncResult(task_id)
        if not task_result.ready():
            # either of these states means the task did not start or did not finish
            # so we mark it as revoked.
            current_app.backend.store_result(task_id, None, "REVOKED")
            revoked_task_ids.append(task_id)
    fractal_logger.info(f"Marked the following tasks as revoked: {task_ids}")
