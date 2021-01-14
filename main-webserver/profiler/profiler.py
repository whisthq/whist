import time
from dataclasses import dataclass

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
    poll_freq=0.5,
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

        print(f"Progress: {round(num_done / 100, 2)}", end="\r")
        if len(to_poll) == 0:
            break

        time.sleep(CONFIG.poll_freq)

    end = time.time()
    print(f"Completed in {end - start} sec")
    print(f"Theoretical optimal on single core: {CONFIG.num_tasks * CONFIG.task_time_ms} sec")


if __name__ == "__main__":
    profile()
