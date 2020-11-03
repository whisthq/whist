from flask import Blueprint, jsonify, request
from flask_jwt_extended import jwt_required

from app import fractalPreProcess
from app.celery.aws_s3_deletion import deleteLogsFromS3
from app.celery.aws_s3_modification import uploadLogsToS3
from app.constants.http_codes import ACCEPTED, BAD_REQUEST
from app.helpers.blueprint_helpers.admin.logs_get import logsHelper
from app.helpers.blueprint_helpers.admin.logs_post import (
    bookmarkHelper,
    unbookmarkHelper,
)
from app.helpers.utils.general.auth import adminRequired

logs_bp = Blueprint("logs_bp", __name__)


@logs_bp.route("/logs/insert", methods=["POST"])
@fractalPreProcess
def logs_post(**kwargs):
    body = kwargs.pop("body")

    try:
        args = (
            body.pop("sender"),
            body.pop("version", None),
            body.pop("connection_id"),
            body.pop("ip", kwargs.pop("received_from")),
            body.pop("identifier"),
            body.pop("secret_key"),
            body.pop("logs"),
        )
    except KeyError:
        return jsonify({"ID": None}), BAD_REQUEST

    task = uploadLogsToS3.delay(*args)

    return jsonify({"ID": task.id}), ACCEPTED


@logs_bp.route("/logs/<action>", methods=("POST",))
@fractalPreProcess
@jwt_required
@adminRequired
def logs_manage(action, **kwargs):
    if action == "delete":
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

    bookmarked = str(bookmarked.upper()) == "TRUE" if bookmarked else False

    output = logsHelper(connection_id, username, bookmarked)

    return jsonify(output), output["status"]
