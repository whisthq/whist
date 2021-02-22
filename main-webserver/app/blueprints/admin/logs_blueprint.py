import logging

from flask import Blueprint, jsonify

from app import fractal_pre_process
from app.celery.aws_s3_modification import upload_logs_to_s3
from app.constants.http_codes import ACCEPTED, BAD_REQUEST
from app.helpers.utils.general.logs import fractal_log

logs_bp = Blueprint("logs_bp", __name__)


@logs_bp.route("/logs", methods=["POST"])
@fractal_pre_process
def logs_post(**kwargs):
    body = kwargs.pop("body")

    try:
        args = (
            body.pop("sender"),
            body.pop("identifier"),
            body.pop("secret_key"),
            body.pop("logs"),
        )
    except KeyError as e:
        fractal_log(
            function="logs",
            label=None,
            logs=f"Error while reading body: {str(e)}",
            level=logging.ERROR,
        )
        return jsonify({"ID": None}), BAD_REQUEST

    task = upload_logs_to_s3.delay(*args)

    return jsonify({"ID": task.id}), ACCEPTED
