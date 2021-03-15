from celery import current_app
from flask import Blueprint, jsonify, make_response
from flask_jwt_extended import jwt_required

from app import fractal_pre_process
from app.celery.dummy import dummy_task
from app.helpers.utils.general.auth import check_developer, developer_required
from app.constants.http_codes import (
    ACCEPTED,
    SUCCESS,
    BAD_REQUEST,
    WEBSERVER_MAINTENANCE,
)

celery_status_bp = Blueprint("celery_status_bp", __name__)


@celery_status_bp.route("/status/<task_id>", methods=["GET"])
@fractal_pre_process
@jwt_required(optional=True)
def celery_status(task_id, **kwargs):  # pylint: disable=unused-argument
    try:
        result = current_app.AsyncResult(task_id)
        if result.status == "SUCCESS":
            response = {"state": result.status, "output": result.result}
            return make_response(jsonify(response), SUCCESS)
        elif result.status == "FAILURE":
            if "MAINTENANCE ERROR" in result.result:
                # this is a special case: the request was accepted but the celery task
                # started after the webserver was put into maintenance mode. We return
                # that webserver is in maintenance mode so client app knows what happened.
                # TODO: can we catch this particular error more cleanly? A cursory search
                # suggested no, see https://stackoverflow.com/questions/35114144/
                # how-can-you-catch-a-custom-exception-from-celery-worker-or-stop-it-being-prefix
                return (
                    jsonify(
                        {
                            "error": "Webserver is in maintenance mode.",
                        }
                    ),
                    WEBSERVER_MAINTENANCE,
                )
            # handle error normally
            msg = None
            if check_developer():
                # give traceback to developers
                msg = f"Experienced an error. Error trace: {result.traceback}"
            else:
                # do not give details to non-developers
                msg = "Experienced an error. Please try the request again."

            response = {"state": result.status, "output": msg}
            return make_response(jsonify(response), SUCCESS)
        else:
            output = result.info
            # if isinstance(result.info, dict):
            #     if "msg" in result.info.keys():
            #         output = result.info["msg"]
            response = {"state": result.status, "output": output}
            return make_response(jsonify(response), SUCCESS)
    except Exception as e:
        response = {
            "state": "FAILURE",
            "output": "Received an Exception that could not be processed: {error}".format(
                error=str(e)
            ),
        }
        return make_response(jsonify(response), BAD_REQUEST)


@celery_status_bp.route("/dummy", methods=["GET"])
@fractal_pre_process
@jwt_required()
@developer_required
def celery_dummy(**kwargs):  # pylint: disable=unused-argument
    task = dummy_task.apply_async([])

    if not task:
        return jsonify({"ID": None}), BAD_REQUEST

    return jsonify({"ID": task.id}), ACCEPTED
