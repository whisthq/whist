from sqlalchemy.exc import DBAPIError

from app import db
from app.constants.http_codes import NOT_FOUND, BAD_REQUEST, SUCCESS, UNAUTHORIZED
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
    base_str = "arn:aws:ecs:us-east-1:747391415460:task-definition/{}"

    return (
        base_str.format(app),
        "us-east-1",
        "demo-cluster",
    )


def modify_region(task_arn, new_region):
    task_arn_lst = task_arn.split(":")
    task_arn_lst[3] = new_region
    return ":".join(task_arn_lst)


def protocol_info(address, port, aeskey):
    """Returns information, which is consumed by the protocol, to the client.

    Arguments:
        address: The IP address of the container whose information should be
            returned.
    """

    response = None, NOT_FOUND
    container = UserContainer.query.filter_by(ip=address, port_32262=port).first()
    if container.secret_key != aeskey:
        print(aeskey, container.secret_key)
        return None, UNAUTHORIZED

    if container:
        response = (
            {
                "allow_autoupdate": container.allow_autoupdate,
                "branch": container.branch,
                "secret_key": container.secret_key,
                "using_stun": container.using_stun,
            },
            SUCCESS,
        )

    return response


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
