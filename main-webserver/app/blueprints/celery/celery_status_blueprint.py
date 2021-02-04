from celery import current_app
from flask import Blueprint, jsonify, make_response
from flask_jwt_extended import jwt_optional

from app import fractal_pre_process
from app.celery.dummy import dummy_task
from app.constants.http_codes import ACCEPTED, BAD_REQUEST
from app.helpers.utils.general.auth import check_developer

celery_status_bp = Blueprint("celery_status_bp", __name__)


@celery_status_bp.route("/status/<task_id>", methods=["GET"])
@fractal_pre_process
@jwt_optional
def celery_status(task_id, **kwargs):  # pylint: disable=unused-argument
    try:
        result = current_app.AsyncResult(task_id)

        if result.status == "SUCCESS":
            response = {"state": result.status, "output": result.result}
            return make_response(jsonify(response), 200)
        elif result.status == "FAILURE":
            msg = None
            if check_developer():
                msg = f"Experienced an error. Error trace: {result.traceback}"
            else:
                # do not give details to non-developers
                msg = "Experienced an error. Please try the request again."

            response = {"state": result.status, "output": msg}
            return make_response(jsonify(response), 200)
        else:
            output = result.info
            # if isinstance(result.info, dict):
            #     if "msg" in result.info.keys():
            #         output = result.info["msg"]
            response = {"state": result.status, "output": output}
            return make_response(jsonify(response), 200)
    except Exception as e:
        response = {
            "state": "FAILURE",
            "output": "Received an Exception that could not be processed: {error}".format(
                error=str(e)
            ),
        }
        return make_response(jsonify(response), 400)


@celery_status_bp.route("/dummy", methods=["GET"])
@fractal_pre_process
def celery_dummy(**kwargs):  # pylint: disable=unused-argument
    task = dummy_task.apply_async([])

    if not task:
        return jsonify({"ID": None}), BAD_REQUEST

    return jsonify({"ID": task.id}), ACCEPTED
