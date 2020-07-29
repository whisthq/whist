from app import *


@celery_instance.task(bind=True)
def uploadLogsToS3(self, sender, connection_id, logs, vm_ip, version):
    """Uploads a log .txt file tos S3

    Args:
        sender (str): server or client
        connection_id (str): Unique connection ID
        logs (str): Log .txt as a string
        vm_ip (str): Server IP
        version (str): Protocol version

    Returns:
        json:
    """

    def S3Upload(content, last_updated, sender, username):
        bucket = "fractal-protocol-logs"

        file_name = "{}-{}".format(
            sender, last_updated.replace("/", "").replace(", ", "-").replace(":", "")
        )
        file_name = "".join(e for e in file_name if e.isalnum())
        file_name = file_name + ".txt"

        s3 = boto3.resource(
            "s3",
            region_name="us-east-1",
            aws_access_key_id=os.getenv("AWS_ACCESS_KEY"),
            aws_secret_access_key=os.getenv("AWS_SECRET_KEY"),
        )

        try:
            s3.Object(bucket, file_name).put(
                Body=content, ACL="public-read", ContentType="text/plain"
            )
            url = "https://fractal-protocol-logs.s3.amazonaws.com/{}".format(file_name)
            return url
        except Exception as e:
            fractalLog(
                function="uploadLogsToS3",
                label=str(username),
                logs="Error uploading {sender} logs to S3: {error}".format(
                    sender=str(sender), error=str(e)
                ),
                level=logging.ERROR,
            )
            return None

    username = None

    try:
        fractalLog(
            function="uploadLogsToS3",
            label=str(username),
            logs="Received {sender} logs from IP {ip_address}".format(
                sender=sender, ip_address=vm_ip
            ),
        )

        sender = sender.upper()
        last_updated = getCurrentTime()
        username = None

        output = fractalSQLSelect(table_name="v_ms", params={"ip": vm_ip})

        vm_info = None
        if output["success"] and output["rows"]:
            vm_info = output["rows"][0]
            username = vm_info["username"]

        file_name = S3Upload(logs, last_updated, sender, username)
        file_name = file_name if file_name else ""

        if sender == "CLIENT":
            output = fractalSQLSelect(
                table_name="logs", params={"connection_id": connection_id}
            )
            if output["success"] and output["rows"]:
                fractalSQLUpdate(
                    table_name="logs",
                    conditional_params={"connection_id": connection_id},
                    new_params={
                        "ip": vm_ip,
                        "last_updated": last_updated,
                        "client_logs": file_name,
                        "username": username,
                    },
                )
            else:
                fractalSQLInsert(
                    table_name="logs",
                    params={
                        "ip": vm_ip,
                        "last_updated": last_updated,
                        "client_logs": file_name,
                        "username": username,
                        "connection_id": connection_id,
                    },
                )
        elif sender == "SERVER":
            output = fractalSQLSelect(
                table_name="logs", params={"connection_id": connection_id}
            )
            if output["success"] and output["rows"]:
                fractalSQLUpdate(
                    table_name="logs",
                    conditional_params={"connection_id": connection_id},
                    new_params={
                        "ip": vm_ip,
                        "last_updated": last_updated,
                        "server_logs": file_name,
                        "username": username,
                        "version": version,
                    },
                )
            else:
                fractalSQLInsert(
                    table_name="logs",
                    params={
                        "ip": vm_ip,
                        "last_updated": last_updated,
                        "server_logs": file_name,
                        "username": username,
                        "connection_id": connection_id,
                        "version": version,
                    },
                )

        return {"status": SUCCESS}
    except Exception as e:
        fractalLog(
            function="uploadLogsToS3",
            label=str(username),
            logs="Error uploading {sender} logs to S3: {error}".format(
                sender=str(sender), error=str(e)
            ),
            level=logging.ERROR,
        )

        return {"status": BAD_REQUEST}
