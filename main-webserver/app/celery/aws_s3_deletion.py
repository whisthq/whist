from typing import Optional, Dict

import boto3
from celery import shared_task

from app.constants.http_codes import SUCCESS
from app.helpers.utils.general.logs import fractal_logger


@shared_task
def delete_logs_from_s3(
    file_name: str, bucket: Optional[str] = "fractal-protocol-logs"
) -> Dict[str, int]:
    """Delete logs from S3

    Args:
        file_name (str): name of file to delete in S3
        bucket (Optional[str]: what bucket the file is in

    Returns:
        json:
    """
    file_name = file_name.replace("https://fractal-protocol-logs.s3.amazonaws.com/", "")

    s3_resource = boto3.resource(
        "s3",
        region_name="us-east-1",
    )

    try:
        s3_resource.Object(bucket, file_name).delete()
        fractal_logger.info("Deleted log {file_name} from S3".format(file_name=file_name))
    except Exception as e:
        fractal_logger.error(
            "Deleting log {file_name} failed: {error}".format(file_name=file_name, error=str(e))
        )

    return {"status": SUCCESS}
