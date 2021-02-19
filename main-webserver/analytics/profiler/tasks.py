import os
import ssl
import time

from celery import Celery


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


loop_iters_ms = estimate_loop_iters_per_ms()
redis_url = os.environ.get("REDIS_URL", "redis://")
celery_kwargs = {
    "broker": redis_url,
    "backend": redis_url,
}

if redis_url.startswith("rediss"):
    celery_kwargs["broker_use_ssl"] = {"ssl_cert_reqs": ssl.CERT_NONE}
    celery_kwargs["redis_backend_use_ssl"] = {"ssl_cert_reqs": ssl.CERT_NONE}

app = Celery("profiler", **celery_kwargs)


class MyTask(app.Task):
    def __call__(self, *args, **kwargs):
        # Pass loop_iters_ms to the task as a keyword argument.
        kwargs["cpu_loop_iters_per_ms"] = loop_iters_ms
        return self.run(*args, **kwargs)


app.Task = MyTask
app.conf.update(
    {
        "broker_connection_retry": False,
        "broker_transport_options": {
            "max_retries": 1,
            "socket_timeout": 1,
        },
        "redis_socket_timeout": 1,
        "result_backend_transport_options": {
            "retry_policy": {
                "max_retries": 1,
            },
        },
    }
)


@app.task
def simulate_cpu_io_task(frac_cpu, total_time_ms, **kwargs):
    """
    Simulate an IO and CPU task.

    Args:
        frac_cpu (float): Number between 0 and 1 indicating how often the task is doing CPU work
        total_time_ms (int): Number of milliseconds to run the task

        kwargs:
            cpu_loop_iters_per_ms (float): Number of loop iterations per ms in the CPU task. Should
                be placed by celery task context.

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
