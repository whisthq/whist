from sqlalchemy.exc import DBAPIError

from app import db
from app.constants.http_codes import NOT_FOUND, BAD_REQUEST, SUCCESS
from app.models.hardware import UserContainer


class BadAppError(Exception):
    """Raised when `preprocess_task_info` doesn't recognized an input."""

    pass


def preprocess_task_info(app):
    """Maps names of applications to ECS task definitions.

    Arguments:
        app: The name of the application for which to generate a task
            definition.

    Returns:
        A  triple containing arguments two through four of the
            create_new_container celery task.
    """

    # TODO: Don't just hard-code the cluster, region, and task definition ARN
    return (
        "arn:aws:ecs:us-east-1:747391415460:task-definition/fractal-browsers-chrome:5",
        "us-east-1",
        "owen-dev-cluster",
    )


def set_stun(user_id, container_id, using_stun):
    """Updates whether or not the specified container should use STUN.

    Arguments:
        user_id: The user_id of the authenticated user.
        container_id: The container_id of the container to modify.
        using_stun: A boolean indicating whether or not the specified container
            should use STUN.
    """

    status = NOT_FOUND
    container = UserContainer.query.get(container_id)

    if getattr(container, "user_id", None) == user_id:
        assert user_id  # Sanity check

        container.using_stun = using_stun

        try:
            db.session.commit()
        except DBAPIError:
            status = BAD_REQUEST
        else:
            status = SUCCESS

    return status
