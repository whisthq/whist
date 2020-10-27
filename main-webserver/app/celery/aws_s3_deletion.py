import logging

import boto3

from celery import shared_task

from app.constants.config import AWS_ACCESS_KEY, AWS_SECRET_KEY
from app.constants.http_codes import SUCCESS
from app.helpers.utils.general.logs import fractalLog
from app.models import db, ProtocolLog


@shared_task(bind=True)
def deleteLogsFromS3(sender, connection_id):
    """Delete logs from S3

    Args:
        connection_id (str): Unique connection ID

    Returns:
        json:
    """

    def S3Delete(file_name):
        bucket = "fractal-protocol-logs"

        file_name = file_name.replace("https://fractal-protocol-logs.s3.amazonaws.com/", "")

        s3 = boto3.resource(
            "s3",
            region_name="us-east-1",
            aws_access_key_id=AWS_ACCESS_KEY,
            aws_secret_access_key=AWS_SECRET_KEY,
        )

        try:
            s3.Object(bucket, file_name).delete()
            fractalLog(
                function="deleteLogsFromS3",
                label="None",
                logs="Deleted log {file_name} from S3".format(file_name=file_name),
            )
        except Exception as e:
            fractalLog(
                function="deleteLogsFromS3",
                label="None",
                logs="Deleting log {file_name} failed: {error}".format(
                    file_name=file_name, error=str(e)
                ),
                level=logging.ERROR,
            )

    logs = ProtocolLog.query.filter_by(connection_id=connection_id).first()

    if logs:
        if logs.server_logs:
            S3Delete(logs.server_logs)

        if logs.client_logs:
            S3Delete(logs.client_logs)

        db.session.delete(logs)
        db.session.commit()

    return {"status": SUCCESS}
