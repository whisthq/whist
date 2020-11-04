from celery import current_app
from flask import Blueprint, jsonify, make_response

from app import fractalPreProcess
from app.celery.dummy import dummyTask
from app.constants.http_codes import ACCEPTED, BAD_REQUEST

celery_status_bp = Blueprint("celery_status_bp", __name__)


@celery_status_bp.route("/status/<task_id>", methods=["GET"])
@fractalPreProcess
def celery_status(task_id, **kwargs):  # pylint: disable=unused-argument
    try:
        result = current_app.AsyncResult(task_id)
        if result.status == "SUCCESS":
            response = {"state": result.status, "output": result.result}
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
@fractalPreProcess
def celery_dummy(**kwargs):  # pylint: disable=unused-argument
    task = dummyTask.apply_async([])

    if not task:
        return jsonify({"ID": None}), BAD_REQUEST

    return jsonify({"ID": task.id}), ACCEPTED
