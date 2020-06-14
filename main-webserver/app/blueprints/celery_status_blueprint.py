from app import *

celery_status_bp = Blueprint("celery_status_bp", __name__)


@celery_status_bp.route("/status/<task_id>", methods=["GET"])
@fractalPreProcess
def celery_status(task_id, **kwargs):
    try:
        result = celery.AsyncResult(task_id)
        if result.status == "SUCCESS":
            response = {
                "state": result.status,
                "output": result.result,
            }
            return make_response(jsonify(response), 200)
        elif result.status == "FAILURE":
            response = {"state": result.status, "output": result.info}
            return make_response(jsonify(response), 200)
        elif result.status == "PENDING":
            response = {"state": result.status, "output": result.info}
            return make_response(jsonify(response), 200)
        else:
            response = {"state": result.status, "output": None}
            return make_response(jsonify(response), 200)
    except:
        response = {
            "state": "FAILURE",
            "output": "Received an Exception that could not be processed",
        }
        return make_response(jsonify(response), 400)
