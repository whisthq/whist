import logging

from flask import Blueprint, jsonify, request
from flask_jwt_extended import jwt_required

from app import fractal_pre_process
from app.celery.aws_s3_deletion import delete_logs_from_s3
from app.celery.aws_s3_modification import upload_logs_to_s3
from app.constants.http_codes import ACCEPTED, BAD_REQUEST, NOT_FOUND
from app.helpers.blueprint_helpers.admin.logs_get import logs_helper
from app.helpers.blueprint_helpers.admin.logs_post import (
    bookmark_helper,
    unbookmark_helper,
)
from app.helpers.utils.general.auth import admin_required
from app.helpers.utils.general.logs import fractal_log

logs_bp = Blueprint("logs_bp", __name__)

@logs_bp.route("/logs", methods=["POST"])
@fractal_pre_process
def logs_post(**kwargs):
    body = kwargs.pop("body")

    try:
        args = (
            body.pop("sender"),
            body.pop("ip", kwargs.pop("received_from")),
            body.pop("identifier"),
            body.pop("secret_key"),
            body.pop("logs"),
        )
    except KeyError as e:
        fractal_log(
            function="logs_post",
            label=None,
            logs=f"Error while reading body: {str(e)}",
            level=logging.ERROR,
        )
        return jsonify({"ID": None}), BAD_REQUEST

    task = upload_logs_to_s3.delay(*args)

    return jsonify({"ID": task.id}), ACCEPTED

