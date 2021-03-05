import time
import boto3

from celery import shared_task
from flask import current_app
from app.constants.http_codes import SUCCESS

from app.helpers.utils.general.logs import fractal_logger
from app.models import UserContainer
from app.exceptions import ContainerNotFoundException

BUCKET_NAME = "fractal-protocol-logs"


class BadSenderError(Exception):
    """Raised by upload_logs_to_s3.

    Raised by upload_logs_to_s3 when the sender argument is anything other than
    "client" or "server" (case-insensitive).
    """


@shared_task
def upload_logs_to_s3(sender, container_id, aes_key, message):
    """Upload logs to S3.

    Arguments:
        sender (str): Either "client" or "server" (case-insensitive).
        container_id (str):  the ARN of the container
        aes_key (str): A secret key that the client uses to authenticate with
            the container and the container uses to authenticate with the web
            server.
        message (str): The log message to save to S3.

    Returns:
        None
    """

    source = sender.upper()

    # Perform input validation on the sender argument.
    if source not in ("CLIENT", "SERVER"):
        fractal_logger.error(f"Unrecognized sender {sender}")
        raise BadSenderError(sender)

    container = UserContainer.query.get(container_id)

    # Make sure that the container with the specified networking attributes
    # exists.
    if not container or aes_key.lower() != container.secret_key.lower():
        if container:
            message = f"Expected secret key {container.secret_key}. Got {aes_key}."
        else:
            message = f"Container {container} does not exist."

        fractal_logger.error(message, extra={"label": container})

        if "test" in container_id and not current_app.testing:
            fractal_logger.error(
                "Test container attempted to communicate with nontest server",
                extra={"label": container_id},
            )
            return {"status": SUCCESS}

        raise ContainerNotFoundException(container_id)

    # Do logging.
    username = str(container.user_id)
    updated_at = str(round(time.time()))
    filename = f"{source}_{username}_{updated_at}.txt"
    s3_resource = boto3.resource("s3")
    s3_object = s3_resource.Object(BUCKET_NAME, filename)

    try:
        s3_object.put(ACL="private", Body=message, ContentType="text/plain")
    except Exception as e:  # TODO: Handle specfic exceptions.
        fractal_logger.error(
            f"Error uploading {sender.lower()} logs to S3: {e}", extra={"label": username}
        )

        raise e

    # Logs have been successfully uploaded to S3. Now we save to the database a
    # pointer to the S3 object.
    url = f"https://{BUCKET_NAME}.s3.amazonaws.com/{filename}"

    fractal_logger.info(f"Protocol server logs: {url}", extra={"label": username})

    return {"status": SUCCESS}
