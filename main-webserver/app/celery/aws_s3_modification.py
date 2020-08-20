from app import *

from app.models.hardware import *
from app.models.logs import *

from app.serializers.hardware import *
from app.serializers.logs import *

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
            aws_access_key_id=AWS_ACCESS_KEY,
            aws_secret_access_key=AWS_SECRET_KEY,
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

        vm = UserVM.query.filter_by(ip=vm_ip).first()

        vm_info = None
        if vm:
            username = vm.user_id

        file_name = S3Upload(logs, last_updated, sender, username)
        file_name = file_name if file_name else ""

        if sender == "CLIENT":
            log = ProtocolLog.query.get(connection_id)
            if log:
                log.ip = vm_ip
                log.timestamp = last_updated
                log.client_logs = file_name
                log.user_id = username
                db.session.commit()
            else:
                log = ProtocolLog(ip=vm_ip, timestamp=last_updated, client_logs=file_name, user_id=username, connection_id=connection_id)
                db.session.add(log)
                db.session.commit()
        elif sender == "SERVER":
            log = ProtocolLog.query.get(connection_id)
            if log:
                log.ip = vm_ip
                log.timestamp = last_updated
                log.server_logs = file_name
                log.user_id = username
                log.version = version
                db.session.commit()
            else:
                log = ProtocolLog(ip=vm_ip, timestamp=last_updated, server_logs=file_name, user_id=username, connection_id=connection_id, version=version)
                db.session.add(log)
                db.session.commit()

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
