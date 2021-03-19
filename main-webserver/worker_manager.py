"""
This file is a replacement for the celery multi feature. We use it because it is
easier to export logs if all workers share stdout/stderr (which this file sets up). 
Also, any signal sent to the process created in this file is automatically sent to
all workers.
"""

import signal
import subprocess
import argparse

from app.helpers.utils.general.logs import fractal_logger


class WorkerManagerSignalHandler:
    """
    Handles signals sent to WorkerManager by just making sure they don't kill us.
    """

    def __init__(self):
        signal.signal(signal.SIGINT, self.handle_signal)
        signal.signal(signal.SIGTERM, self.handle_signal)

    def handle_signal(self, signum, frame):  # pylint: disable=no-self-use,unused-argument
        """
        Make sure signal does not kill WorkerManager. All signals are also sent to children
        because they are in this process group.
        """
        pass


def run_worker_manager(num_workers: int, pool: str, worker_concurrency: int):
    """
    Start a thin parent process that starts `num_workers` celery workers with this command:

    celery -A tasks worker --pool <pool> --concurrency <worker_concurrency> --loglevel INFO

    Any signals sent to this process go to all the workers. Stdout/stderr are shared by
    WorkerManager (this process) and all children.

    Args:
        num_workers: number of celery workers to start
        pool: pooling option to pass to celery
        worker_concurrency: concurrency of each worker
    """
    children = []
    for wid in range(num_workers):
        cmd = (
            f"celery -A entry.celery worker --pool {pool} --concurrency {worker_concurrency} "
            f"--loglevel INFO -n worker_{wid}@%h"
        )
        child = subprocess.Popen(cmd, shell=True, close_fds=False)
        children.append(child)

    # capture signals now that children are running
    WorkerManagerSignalHandler()

    children_pids = [child.pid for child in children]
    fractal_logger.info(f"Waiting for children {children_pids} to finish...")

    bad_exit_code = False
    for child in children:
        child.wait()
        if child.returncode != 0:
            bad_exit_code = True
            fractal_logger.error(f"Child with pid {child.pid} exited with {child.returncode}")

    if bad_exit_code:
        fractal_logger.fatal("A child failed so WorkerManager is exiting with error...")

    fractal_logger.info("All children have exited successfully. WorkerManager is exiting...")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Run several celery workers that share stdout/stderr."
    )

    parser.add_argument(
        "--num_workers", type=int, required=True, help="Number of celery workers to start."
    )
    parser.add_argument("--pool", type=str, required=True, help="Celery pooling option to run.")
    parser.add_argument(
        "--worker_concurrency", type=int, required=True, help="Concurrency for each worker."
    )
    args = parser.parse_args()
    run_worker_manager(args.num_workers, args.pool, args.worker_concurrency)
