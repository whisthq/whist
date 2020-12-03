from flask import Blueprint, jsonify, request

from app import fractal_pre_process
from app.constants.http_codes import SUCCESS

from app.helpers.blueprint_helpers.admin.hasura_get import auth_helper

from app.helpers.utils.general.logs import fractal_log

hasura_bp = Blueprint("hasura_bp", __name__)


@hasura_bp.route("/hasura/auth", methods=["GET"])
@fractal_pre_process
def hasura_auth_get(**kwargs):  # pylint: disable=unused-argument
    token = request.headers.get("Authorization")
    output = auth_helper(token)

    fractal_log("", "", str(output))

    return jsonify(output), SUCCESS
