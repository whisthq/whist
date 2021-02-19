import os
import time
import ssl

from celery import Celery, shared_task

from utils import get_redis_url


def make_celery():
    """
    Returns a Celery object with initialized redis parameters.
    """
    # TODO: replace this with get_redis_url when stuff merges
    redis_url = get_redis_url()

    app = None
    if redis_url[:6] == "rediss":
        # use SSL
        app = Celery(
            "",
            broker=redis_url,
            backend=redis_url,
            broker_use_ssl={
                "ssl_cert_reqs": ssl.CERT_NONE,
            },
            redis_backend_use_ssl={
                "ssl_cert_reqs": ssl.CERT_NONE,
            },
        )

    elif redis_url[:5] == "redis":
        # use regular
        app = Celery(
            "",
            broker=redis_url,
            backend=redis_url,
        )

    else:
        # unexpected input, fail out
        raise ValueError(f"Unexpected prefix in redis url: {redis_url}")

    # allow many redis connections
    app.conf.update(
        celery_redis_max_connections=1000,
    )

    TaskBase = app.Task
    loop_iters_ms = estimate_loop_iters_per_ms()

    class ContextTask(TaskBase):
        def __call__(self, *args, **kwargs):
            # pass loop_iters_ms to the task as a keyword arg
            kwargs["cpu_loop_iters_per_ms"] = loop_iters_ms
            return TaskBase.__call__(self, *args, **kwargs)

    app.Task = ContextTask

    return app


def estimate_loop_iters_per_ms():
    """
    Figures out how many loop iterations take roughly a ms on current machine.

    Returns:
        (float) number of loop iters
    """

    start = time.time()
    guess = 100000
    _ = 0
    for i in range(guess):
        _ = i
    end = time.time()

    time_ms = (end - start) * 1000
    # this scales guess according to how much time the guess took
    return guess / time_ms


celery_app = make_celery()


@celery_app.task
def simulate_cpu_io_task(frac_cpu, total_time_ms, **kwargs):
    """
    Simulate an IO and CPU task.

    Args:
        frac_cpu (float): Number between 0 and 1 indicating how often the task is doing CPU work
        total_time_ms (int): Number of milliseconds to run the task

        kwargs:
            cpu_loop_iters_per_ms (float): Number of loop iterations per ms in the CPU task. Should be placed
                by celery task context.

    Returns:
        None
    """

    if "cpu_loop_iters_per_ms" not in kwargs:
        raise ValueError("Need cpu_loop_iters_per_ms in kwargs to run")

    loop_iters_per_ms = kwargs["cpu_loop_iters_per_ms"]
    cpu_loops = int(frac_cpu * loop_iters_per_ms)
    io_timeshare = (1.0 - frac_cpu) * 0.001

    for _ in range(total_time_ms):
        chew_cpu_cycles(cpu_loops)
        time.sleep(io_timeshare)


def chew_cpu_cycles(iters):
    """
    Does a simple for loop for the number of iterations given.

    Args:
        (int) iters

    Returns:
        None
    """
    _ = 0
    for i in range(iters):
        _ = i
