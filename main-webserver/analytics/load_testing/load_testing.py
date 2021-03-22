import logging
import time


def simulate_task(*args, **kwargs):
    from analytics.profiler.utils import CPU_LOOP_ITERS_PER_S, simulate_cpu_io_task

    frac_cpu = 0.05
    total_time_s = 1
    simulate_cpu_io_task(frac_cpu, total_time_s, CPU_LOOP_ITERS_PER_S, chunks=5)


def init_load_tester(flask_app):
    from app.helpers.utils.general.logs import fractal_logger
    from app.celery.aws_ecs_creation import _assign_container

    if not (
        flask_app.config["LOAD_TESTING"] == "True" or flask_app.config["LOAD_TESTING"] == "true"
    ):
        fractal_logger.error("Tried to start load tester even though LOAD_TESTING is not True.")
        return

    setattr(_assign_container, "__code__", simulate_task.__code__)
