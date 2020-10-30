import time

from celery import shared_task

from app.constants.http_codes import SUCCESS


@shared_task(bind=True)
def dummyTask(self):
    self.update_state(
        state="PENDING",
        meta={
            "msg": "Running performance tests. This could take a few extra minutes.",
            "progress": 10,
        },
    )

    time.sleep(10)

    self.update_state(
        state="PENDING", meta={"msg": "Spinning the wheels.", "progress": 40,},
    )

    time.sleep(15)

    self.update_state(
        state="PENDING", meta={"msg": "Oiling the tubes.", "progress": 70,},
    )

    time.sleep(2)

    return {"status": SUCCESS}
