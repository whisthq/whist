import time

from celery import shared_task

from app.constants.http_codes import SUCCESS
from app.helpers.utils.general.logs import fractal_logger


@shared_task(bind=True)
def dummy_task(self):
    self.update_state(
        state="STARTED",
        meta={
            "msg": "Running performance tests. This could take a few extra minutes.",
            "progress": 10,
        },
    )
    fractal_logger.info("Running dummy_task...")

    time.sleep(10)

    self.update_state(
        state="STARTED",
        meta={
            "msg": "Spinning the wheels.",
            "progress": 40,
        },
    )

    time.sleep(15)

    self.update_state(
        state="STARTED",
        meta={
            "msg": "Greasing the gears.",
            "progress": 70,
        },
    )

    time.sleep(2)

    return {"status": SUCCESS}
