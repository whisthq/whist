from app import *


@celery_instance.task(bind=True)
def deleteLogsFromS3(sender, connection_id):
    """Delete logs from S3

    Args:
        connection_id (str): Unique connection ID

    Returns:
        json:
    """

    def S3Delete(file_name, last_updated):
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
            fractalLog(
                function="deleteLogsFromS3",
                label="None",
                logs="Deleting log {file_name} from S3".format(file_name=file_name)
            )
            s3.Object(bucket, file_name).delete()
            return True
        except Exception as e:
            print(str(e))
            return False

    last_updated = getCurrentTime()

    with engine.connect() as conn:
        command = text(
            """
			SELECT * FROM logs WHERE "connection_id" = :connection_id
			"""
        )

        params = {"connection_id": connection_id}
        logs_found = (cleanFetchedSQL(conn.execute(command, **params).fetchall()))[0]
        success_serverlogs = None

        # delete server log for this connection ID
        if logs_found["server_logs"]:
            success_serverlogs = S3Delete(logs_found["server_logs"], last_updated)
            if success_serverlogs:
                print("Successfully deleted log: " + str(logs_found["server_logs"]))
            else:
                print("Could not delete log: " + str(logs_found["server_logs"]))

        # delete the client logs
        if logs_found["client_logs"]:
            print(logs_found["client_logs"])
            success_clientlogs = S3Delete(logs_found["client_logs"], last_updated)
            if success_clientlogs:
                print("Successfully deleted log: " + str(logs_found["client_logs"]))
            else:
                print("Could not delete log: " + str(logs_found["client_logs"]))

        command = text(
            """
			DELETE FROM logs WHERE "connection_id" = :connection_id
			"""
        )

        params = {"connection_id": connection_id}
        conn.execute(command, **params)

        conn.close()
        return 1
    return -1
