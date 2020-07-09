from app import *
from app.celery.aws_s3_modification import *

logs_bp = Blueprint("logs_bp", __name__)


@logs_bp.route("/logs", methods=["POST"])
@fractalPreProcess
def logs_post(**kwargs):
    version = None
    if "version" in kwargs["body"].keys():
        version = kwargs["body"]["version"]

    vm_ip = kwargs["received_from"]
    if "vm_ip" in kwargs["body"].keys():
        vm_ip = kwargs["body"]["vm_ip"]

    task = uploadLogsToS3.apply_async(
        [
            kwargs["body"]["sender"],
            kwargs["body"]["connection_id"],
            kwargs["body"]["logs"],
            vm_ip,
            version,
        ]
    )

    if not task:
        return jsonify({"ID": None}), BAD_REQUEST

    return jsonify({"ID": task.id}), ACCEPTED
