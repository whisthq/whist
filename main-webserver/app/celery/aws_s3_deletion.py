from app import *


@celery_instance.task(bind=True)
def deleteLogsFromS3(sender, connection_id):
    """Delete logs from S3

    Args:
        connection_id (str): Unique connection ID

    Returns:
        json:
    """

    def S3Delete(file_name):
        bucket = "fractal-protocol-logs"

        file_name = file_name.replace(
            "https://fractal-protocol-logs.s3.amazonaws.com/", ""
        )

        s3 = boto3.resource(
            "s3",
            region_name="us-east-1",
            aws_access_key_id=os.getenv("AWS_ACCESS_KEY"),
            aws_secret_access_key=os.getenv("AWS_SECRET_KEY"),
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

    output = fractalSQLSelect(
        table_name="logs", params={"connection_id": connection_id}
    )

    if output["success"] and output["rows"]:
        logs = output["rows"][0]
        if logs["server_logs"]:
            S3Delete(logs_found["server_logs"])

        if logs_found["client_logs"]:
            S3Delete(logs_found["client_logs"])

        fractalSQLDelete(table_name="logs", params={"connection_id": connection_id})

    return {"status": SUCCESS}
