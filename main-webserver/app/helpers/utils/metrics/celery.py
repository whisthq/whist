from typing import Optional, Mapping
import time
from celery import signals, states, Task
from flask.app import Flask

from app.helpers.utils.metrics import monotonic_duration_ms
from app.helpers.utils.metrics.flask_app import app_record_metrics
import app.helpers.utils.metrics.keys as mkey
import app.helpers.utils.metrics.keys_dims as dkey


# This should be a globally unique key that shouldn't conflict with any other
# potential keys in tasks.request.headers since it will be used to store a
# timestamp for when the task began processing
HEADER__CELERY_EXECUTION_START = "Fractal-Execution-StartTime"


def _get_dims_from_task(task: Task) -> Mapping[str, str]:
    """Extract relevant dimensions to send along with the metrics for a given task, such as
    the task name and ID.
    """
    return {
        dkey.CELERY_TASK_NAME: task.name,
        dkey.CELERY_TASK_ID: task.request.id,
        dkey.CELERY_WORKER_ID: task.request.hostname,
    }


def _make_prerun_handler_for_app(flask_app: Flask):
    """Returns a postrun handler that has access to the app context from flask_app"""

    def prerun_handler(task: Task, **kwargs) -> None:  # pylint: disable=unused-argument
        """
        This isn't registered using the @signals.task_prerun.connect decorator so that importing
        this module doesn't introduce side effects. Instead, an explicit call to the register
        function needs to be made.
        """
        # It's okay to use time.monotonic here because the pre-run handler and post-run
        # handler are run in the same worker, and therefore the same process, so the monotonic
        # values will be consistent and comparable. time.monotonic could not be used if this
        # header needed to be valid outside of a single process, since it returns an opaque
        # value that is only valuable for comparing against itself to calculate a duration.
        setattr(task.request, HEADER__CELERY_EXECUTION_START, time.monotonic())
        # TODO: capture CELERY__TASK_QUEUE_DURATION__MS by tagging tasks with a timestamp when
        # they're first submitted
        with flask_app.app_context():
            app_record_metrics(
                {mkey.CELERY__TASKS_PROCESSING__COUNT: 1}, extra_dims=_get_dims_from_task(task)
            )

    return prerun_handler


def _make_postrun_handler_for_app(flask_app: Flask):
    """Returns a postrun handler that has access to the app context from flask_app"""

    def postrun_handler(
        task: Task, state: Optional[str] = None, **kwargs  # pylint: disable=unused-argument
    ) -> None:
        """
        This isn't registered using the @signals.task_prerun.connect decorator so that importing
        this module doesn't introduce side effects. Instead, an explicit call to the register
        function needs to be made.
        """
        metrics = {mkey.CELERY__TASKS_PROCESSING__COUNT: -1}
        if start_time := getattr(task.request, HEADER__CELERY_EXECUTION_START, None):
            runtime = monotonic_duration_ms(start_time, time.monotonic())
            metrics[mkey.CELERY__TASK_DURATION__MS] = runtime
        dimensions = {**_get_dims_from_task(task)}
        if state == states.SUCCESS:
            dimensions[dkey.CELERY_TASK_STATUS] = "success"
        elif state == states.FAILURE:
            dimensions[dkey.CELERY_TASK_STATUS] = "failure"
        with flask_app.app_context():
            app_record_metrics(metrics, extra_dims=dimensions)

    return postrun_handler


def register_metrics_monitor_in_worker(flask_app: Flask) -> None:
    """Listen to Celery events to automatically report task metrics from each worker.

    The following metrics will be collected:
    * # of celery tasks being processed will be incremented/decremented when the task starts/ends
    * milliseconds a successful task took to complete
    * milliseconds a failing task took to terminate

    Each of these metrics will be accompanied by the standard dimensions plus:
    * the task's ID
    * the task's name
    """
    # For reference (since the Celery docs are sparse around such details like which process
    # this signal handler is executed on) here's the location where the relevant signal handlers
    # are triggered:
    # - task_prerun @ https://github.com/celery/celery/blob/e62dc67/celery/app/trace.py#L404
    # - task_postrun # https://github.com/celery/celery/blob/e62dc67/celery/app/trace.py#L511
    # - build_tracer.apply, which what eventually calls the two previous functions
    #     @ https://github.com/celery/celery/blob/e62dc67/celery/app/trace.py#L404
    # NOTE: Called with `weak=False` since:
    # > Django [Celery] stores signal handlers as weak references by default. Thus, if your
    # > receiver is a local function, it may be garbage collected. To prevent this, pass
    # > weak=False when you call the signal’s connect() method.
    # https://docs.djangoproject.com/en/3.1/topics/signals/
    # Django signals are the basis of celery signals.
    signals.task_prerun.connect(_make_prerun_handler_for_app(flask_app), weak=False)
    signals.task_postrun.connect(_make_postrun_handler_for_app(flask_app), weak=False)
