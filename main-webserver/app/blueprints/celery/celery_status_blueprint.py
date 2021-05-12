from uuid import UUID

from celery import current_app
from flask import abort, Blueprint, jsonify, make_response

from app import fractal_pre_process
from app.celery.dummy import dummy_task
from app.helpers.utils.general.auth import developer_required
from app.constants.http_codes import (
    ACCEPTED,
    SUCCESS,
    BAD_REQUEST,
    WEBSERVER_MAINTENANCE,
)
from auth0 import has_scope

celery_status_bp = Blueprint("celery_status_bp", __name__)


@celery_status_bp.route("/status/<task_id>", methods=["GET"])
@fractal_pre_process
def celery_status(task_id, **kwargs):  # pylint: disable=unused-argument
    try:
        # The UUID constuctor will attempt to parse the supplied task ID, treated as a hex string,
        # into a UUID. Hex strings that are parseable by the UUID constructor may optionally
        # contain curly braces, dashes, or the prefix "urn:uuid:". Under the hood, the UUID
        # constructor strips the input hex string of all optional characters and, ensures that
        # exactly 32 characters remain, and finally attempts to convert them into an integer.
        # https://docs.python.org/3/library/uuid.html#uuid.UUID
        # https://github.com/python/cpython/blob/bdee2a389e4b10e1c0ab65bbd4fd03defe7b2837/Lib/uuid.py#L167
        UUID(hex=task_id)
    except ValueError:
        abort(make_response({"error": "Invalid task ID"}, BAD_REQUEST))

    # methods like .status, .result of AsyncResult are actually properties that are fetched
    # on each invocation. This led to a race where we mix returns from a non-SUCCESS state
    # and a SUCCESS state. See https://github.com/fractal/fractal/pull/1725
    result = current_app.AsyncResult(task_id)
    result_data = result._get_task_meta()  # pylint: disable=protected-access
    if result_data["status"] == "SUCCESS":
        response = {"state": result_data["status"], "output": result_data["result"]}
        return make_response(jsonify(response), SUCCESS)
    elif result_data["status"] == "FAILURE":
        if "MAINTENANCE ERROR" in str(result_data["result"]):
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

        if has_scope("admin"):
            # give traceback to developers
            msg = f"Experienced an error. Error trace: {result_data.get('traceback')}"
        else:
            # do not give details to non-developers
            msg = "Experienced an error. Please try the request again."

        response = {"state": result_data["status"], "output": msg}
        return make_response(jsonify(response), SUCCESS)
    else:
        output = result_data["result"]
        response = {"state": result_data["status"], "output": output}
        return make_response(jsonify(response), SUCCESS)


@celery_status_bp.route("/dummy", methods=["GET"])
@fractal_pre_process
@developer_required
def celery_dummy(**kwargs):  # pylint: disable=unused-argument
    task = dummy_task.apply_async([])

    if not task:
        return jsonify({"ID": None}), BAD_REQUEST

    return jsonify({"ID": task.id}), ACCEPTED
