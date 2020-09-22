from app import *
from app.celery.dummy import *

celery_status_bp = Blueprint("celery_status_bp", __name__)


@celery_status_bp.route("/status/<task_id>", methods=["GET"])
@fractalPreProcess
def celery_status(task_id, **kwargs):
    try:
        result = celery_instance.AsyncResult(task_id)
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
def celery_dummy(**kwargs):
    task = dummyTask.apply_async([])

    if not task:
        return jsonify({"ID": None}), BAD_REQUEST

    return jsonify({"ID": task.id}), ACCEPTED
