from flask import Blueprint, jsonify, request

from app import fractal_pre_process
from app.constants.http_codes import SUCCESS

from app.helpers.blueprint_helpers.admin.hasura_get import auth_token_helper, login_token_helper

from app.helpers.utils.general.logs import fractal_log

hasura_bp = Blueprint("hasura_bp", __name__)


@hasura_bp.route("/hasura/auth", methods=["GET"])
@fractal_pre_process
def hasura_auth_get(**kwargs):  # pylint: disable=unused-argument
    """The endpoint for the hasura auth hook.

    All headers passed through GraphQL queries will end up here for processing.

    Returns:
        A dictionary containing key value pairs for "X-Hasura-Role", "X-Hasura-User-Id",
        and "X-Hasura-Login-Token" that will be returned to Hasura for permissions checking.

    """
    auth_token = request.headers.get("Authorization")
    auth_output = auth_token_helper(auth_token)

    login_token = request.headers.get("Login")
    login_output = login_token_helper(login_token)

    output = dict(auth_output)
    output.update(login_output)

    fractal_log("", "", str(output))

    return jsonify(output), SUCCESS
