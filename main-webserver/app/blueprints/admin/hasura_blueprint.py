from flask import Blueprint, jsonify, request

from app import fractalPreProcess
from app.constants.http_codes import SUCCESS

from app.helpers.blueprint_helpers.admin.hasura_get import authHelper

from app.helpers.utils.general.logs import fractalLog

hasura_bp = Blueprint("hasura_bp", __name__)


@hasura_bp.route("/hasura/auth", methods=["GET"])
@fractalPreProcess
def hasura_auth_get(**kwargs):  # pylint: disable=unused-argument
    token = request.headers.get("Authorization")
    output = authHelper(token)

    fractalLog("", "", str(output))

    return jsonify(output), SUCCESS
