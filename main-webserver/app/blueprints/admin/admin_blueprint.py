from flask import Blueprint, jsonify

from app import fractal_pre_process
from app.constants.http_codes import SUCCESS, UNAUTHORIZED
from app.helpers.blueprint_helpers.admin.admin_post import admin_login_helper

admin_bp = Blueprint("admin_bp", __name__)


@admin_bp.route("/admin/<action>", methods=["POST"])
@fractal_pre_process
def admin_post(action, **kwargs):
    body = kwargs["body"]
    if action == "login":
        output = None
        if body["username"] and body["password"]:
            output = admin_login_helper(body["username"], body["password"])

        if output:
            return jsonify(output), SUCCESS
        else:
            return jsonify({}), UNAUTHORIZED
