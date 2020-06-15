from app import *

celery_status_bp = Blueprint("celery_status_bp", __name__)


@celery_status_bp.route("/status/<task_id>", methods=["GET"])
@fractalPreProcess
def celery_status(task_id, **kwargs):
    try:
        result = celery_instance.AsyncResult(task_id)
        if result.status == "SUCCESS":
            response = {
                "state": result.status,
                "output": result.result,
            }
            return make_response(jsonify(response), 200)
        else:
            output = result.info
            if isinstance(result.info, dict):
                if "msg" in result.info.keys():
                    output = result.info["msg"]
            response = {"state": result.status, "output": str(output)}
            return make_response(jsonify(response), 200)
    except Exception as e:
        response = {
            "state": "FAILURE",
            "output": "Received an Exception that could not be processed: {error}".format(
                error=str(e)
            ),
        }
        return make_response(jsonify(response), 400)
