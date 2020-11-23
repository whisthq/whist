from flask import Blueprint, jsonify

from app import fractal_pre_process
from app.constants.http_codes import NOT_FOUND
from app.helpers.blueprint_helpers.auth.google_auth_post import (
    login_helper,
    reason_helper,
)

google_auth_bp = Blueprint("google_auth_bp", __name__)


@google_auth_bp.route("/google/<action>", methods=["POST"])
@fractal_pre_process
def google_auth_post(action, **kwargs):
    if action == "login":
        code = kwargs["body"]["code"]

        client_app = False
        if "clientApp" in kwargs["body"].keys():
            client_app = kwargs["body"]["clientApp"]

        output = login_helper(code, client_app)

        return jsonify(output), output["status"]

    if action == "reason":
        username, reason_for_signup = (kwargs["body"]["username"], kwargs["body"]["reason"])

        output = reason_helper(username, reason_for_signup)

        return jsonify(output), output["status"]

    return jsonify({"error": "Not Found"}), NOT_FOUND
