import time

from app.models import (
    db,
    InstanceInfo,
)

from app.helpers.utils.general.logs import fractal_logger

MAX_POLL_ITERATIONS = 600
POLL_SLEEP_INTERVAL = 10


def _poll(instance_id: str) -> bool:
    """Poll the database until the web server receives its first ping from the new instance.
    Time out after MAX_POLL_ITERATIONS seconds.
    This may be an appropriate use case for Hasura subscriptions.
    This function should patched to immediately return True in order to get CI to pass.
    Arguments:
        instance_id: The cloud provider ID of the instance whose state to poll.
    Returns:
        True if and only if the instance's starts with ACTIVE by the end of the polling period.
    """

    instance = InstanceInfo.query.get(instance_id)
    result = False

    for i in range(MAX_POLL_ITERATIONS):
        if not instance.status == "ACTIVE":
            fractal_logger.warning(
                f"{instance.cloud_provider_id} deployment in progress. {i}/{MAX_POLL_ITERATIONS}"
            )
            time.sleep(POLL_SLEEP_INTERVAL)
            db.session.refresh(instance)
        else:
            result = True
            break

    return result
