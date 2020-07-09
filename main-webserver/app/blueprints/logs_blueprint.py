from app import *

from app.helpers.blueprint_helpers.logs_get import *
from app.helpers.blueprint_helpers.logs_post import *

from app.celery.aws_s3_modification import *
from app.celery.aws_s3_deletion import *

logs_bp = Blueprint("logs_bp", __name__)


@logs_bp.route("/logs/<action>", methods=["POST"])
@fractalPreProcess
def logs_post(action, **kwargs):
    if action == "insert":
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

    elif action == "delete":
        connection_id = kwargs["body"]["connection_id"]

        task = deleteLogsFromS3.apply_async([connection_id])

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED

    elif action == "bookmark":
        connection_id = kwargs["body"]["connection_id"]

        output = bookmarkHelper(connection_id)

        return jsonify(output), output["status"]

    elif action == "unbookmark":
        connection_id = kwargs["body"]["connection_id"]

        output = unbookmarkHelper(connection_id)

        return jsonify(output), output["status"]


@logs_bp.route("/logs", methods=["GET"])
@fractalPreProcess
@jwt_required
@adminRequired
def logs_get(**kwargs):
    connection_id, username, bookmarked = (
        request.args.get("connection_id"),
        request.args.get("username"),
        request.args.get("bookmarked"),
    )

    bookmarked = str(bookmarked.upper()) == "TRUE"

    output = logsHelper(connection_id, username, bookmarked)

    return jsonify(output), output["status"]

