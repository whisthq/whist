import time
from dataclasses import dataclass
import sys
import argparse
import os

from tasks import celery_app, estimate_loop_iters_per_ms, simulate_cpu_io_task


@dataclass
class ProfileConfig:
    num_tasks: int
    frac_cpu: float
    task_time_ms: int
    poll_freq: float


CONFIG = ProfileConfig(
    num_tasks=100,
    frac_cpu=0.1,
    task_time_ms=100,
    poll_freq=0.1,
)


def profile():
    task_ids = list()
    for _ in range(CONFIG.num_tasks):
        task_id = simulate_cpu_io_task.delay(CONFIG.frac_cpu, CONFIG.task_time_ms)
        task_ids.append(task_id.id)

    to_poll = set(task_ids)
    start = time.time()
    num_done = 0
    while True:
        for tid in list(to_poll):
            res = celery_app.AsyncResult(tid)
            if res.status == "SUCCESS":
                to_poll.remove(tid)
                num_done += 1

        print(f"Progress: {round(num_done / CONFIG.num_tasks, 2)}%", end="\r")
        if len(to_poll) == 0:
            break

        time.sleep(CONFIG.poll_freq)

    end = time.time()
    print(f"Completed in {end - start} sec")
    print(
        f"Theoretical optimal on single core with no timeshare yielding: {CONFIG.num_tasks * CONFIG.task_time_ms / 1000} sec"
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run a profiler.")

    parser.add_argument("--num_tasks", type=int, default=100, help="Number of tasks to run.")
    parser.add_argument(
        "--frac_cpu", type=float, default=0.1, help="Fraction of time spent doing CPU work."
    )
    parser.add_argument(
        "--task_time_ms", type=int, default=100, help="How long a task should take, in ms."
    )
    parser.add_argument(
        "--poll_freq", type=float, default=0.5, help="How often to poll for results, in sec."
    )

    args = parser.parse_args()

    CONFIG.num_tasks = args.num_tasks
    CONFIG.frac_cpu = args.frac_cpu
    CONFIG.task_time_ms = args.task_time_ms
    CONFIG.poll_freq = args.poll_freq

    profile()
