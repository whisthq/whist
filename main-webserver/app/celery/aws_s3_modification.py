import logging
import time
import boto3

from celery import shared_task
from app.constants.http_codes import SUCCESS

from app.helpers.utils.general.logs import fractal_log
from app.models import UserContainer

BUCKET_NAME = "fractal-protocol-logs"


class BadSenderError(Exception):
    """Raised by upload_logs_to_s3.

    Raised by upload_logs_to_s3 when the sender argument is anything other than
    "client" or "server" (case-insensitive).
    """


class ContainerNotFoundError(Exception):
    """Raised by upload_logs_to_s3.

    Raised by upload_logs_to_s3 when the container identified by the ip and port
    arguments does not exist.
    """

    def __init__(self, ip, port):
        super().__init__(f"IP: {ip}, port: {port}")


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
        fractal_log(
            function="upload_logs_to_s3",
            label=None,
            logs=f"Unrecognized sender {sender}",
            level=logging.ERROR,
        )

        raise BadSenderError(sender)

    container = UserContainer.query.get(container_id)

    # Make sure that the container with the specified networking attributes
    # exists.
    if not container or aes_key.lower() != container.secret_key.lower():
        if container:
            message = f"Expected secret key {container.secret_key}. Got {aes_key}."
        else:
            message = f"Container {container} does not exist."

        fractal_log(
            function="upload_logs_to_s3",
            label=container,
            logs=message,
            level=logging.ERROR,
        )

        raise ContainerNotFoundError(ip, port)

    # Do logging.
    username = str(container.user_id)
    updated_at = str(round(time.time()))
    filename = f"{source}_{username}_{updated_at}.txt"
    s3_resource = boto3.resource("s3")
    s3_object = s3_resource.Object(BUCKET_NAME, filename)

    try:
        s3_object.put(ACL="private", Body=message, ContentType="text/plain")
    except Exception as e:  # TODO: Handle specfic exceptions.
        fractal_log(
            function="upload_logs_to_s3",
            label=username,
            logs=f"Error uploading {sender.lower()} logs to S3: {e}",
            level=logging.ERROR,
        )

        raise e

    # Logs have been successfully uploaded to S3. Now we save to the database a
    # pointer to the S3 object.
    url = f"https://{BUCKET_NAME}.s3.amazonaws.com/{filename}"

    fractal_log(
        function="upload_logs_to_s3",
        label=username,
        logs=f"Protocol server logs: {url}",
    )

    return {"status": SUCCESS}
