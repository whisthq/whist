import logging

import boto3

from celery import shared_task

from app.constants.http_codes import SUCCESS
from app.helpers.utils.general.logs import fractal_log
from app.models import db, ProtocolLog


@shared_task(bind=True)
def delete_logs_from_s3(connection_id):
    """Delete logs from S3

    Args:
        connection_id (str): Unique connection ID

    Returns:
        json:
    """

    def s3_delete(file_name):
        bucket = "fractal-protocol-logs"

        file_name = file_name.replace("https://fractal-protocol-logs.s3.amazonaws.com/", "")

        s3_resource = boto3.resource(
            "s3",
            region_name="us-east-1",
        )

        try:
            s3_resource.Object(bucket, file_name).delete()
            fractal_log(
                function="delete_logs_from_s3",
                label="None",
                logs="Deleted log {file_name} from S3".format(file_name=file_name),
            )
        except Exception as e:
            fractal_log(
                function="delete_logs_from_s3",
                label="None",
                logs="Deleting log {file_name} failed: {error}".format(
                    file_name=file_name, error=str(e)
                ),
                level=logging.ERROR,
            )

    logs = ProtocolLog.query.filter_by(connection_id=connection_id).first()

    if logs:
        if logs.server_logs:
            s3_delete(logs.server_logs)

        if logs.client_logs:
            s3_delete(logs.client_logs)

        db.session.delete(logs)
        db.session.commit()

    return {"status": SUCCESS}
