from flask import Blueprint, jsonify, request
from flask_jwt_extended import get_jwt_identity, jwt_required

hasura_bp = Blueprint("hasura_bp", __name__)


@hasura_bp.route("/hasura/auth")
@jwt_required()
def hasura_auth_get():
    """The endpoint for the hasura auth hook.

    All headers passed through GraphQL queries will end up here for processing.

    Returns:
        A dictionary containing key value pairs for "X-Hasura-Role", "X-Hasura-User-Id",
        and "X-Hasura-Login-Token" that will be returned to Hasura for permissions checking.

    """

    return jsonify(
        {
            "X-Hasura-Login-Token": request.headers.get("X-Hasura-Login-Token", ""),
            "X-Hasura-Role": "user",
            "X-Hasura-User-Id": get_jwt_identity(),
        }
    )
