import time

from celery import shared_task

from app.constants.http_codes import SUCCESS
from app.helpers.utils.general.logs import fractal_logger


@shared_task(bind=True)
def dummy_task(self):
    """
    A dummy task deployed with our webserver. Useful for scripted testing and manually making
    sure celery is working. Ex: test_celery_sigterm, .github/workflows/main-webserver-check-pr.yml
    """
    self.update_state(state="STARTED")
    fractal_logger.info("Running dummy_task...")

    # beware of removing this. test_celery_sigterm uses it and the sleep is critical to the test.
    time.sleep(10)
