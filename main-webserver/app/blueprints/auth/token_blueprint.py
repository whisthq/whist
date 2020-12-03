from flask import Blueprint, jsonify
from flask_jwt_extended import get_jwt_identity, jwt_refresh_token_required, jwt_required

from app import fractal_pre_process, jwtManager
from app.helpers.blueprint_helpers.auth.token_post import validate_token_helper
from app.helpers.utils.general.tokens import get_access_tokens

token_bp = Blueprint("token_bp", __name__)


@token_bp.route("/token/refresh", methods=["POST"])
@jwt_refresh_token_required
@fractal_pre_process
def token(**kwargs):  # pylint: disable=unused-argument
    username = get_jwt_identity()
    access_token, refresh_token = get_access_tokens(username)

    return (
        jsonify(
            {
                "status": 200,
                "username": username,
                "access_token": access_token,
                "refresh_token": refresh_token,
            }
        ),
        200,
    )


@token_bp.route("/token/validate", methods=["GET"])
@fractal_pre_process
@jwt_required
def validate_token(**kwargs):
    output = validate_token_helper()
    return jsonify(output), output["status"]


@jwtManager.expired_token_loader
@fractal_pre_process
def my_expired_token_callback(expired_token, **kwargs):  # pylint: disable=unused-argument
    token_type = expired_token["type"]
    return (jsonify({"status": 401, "msg": "The {} token has expired".format(token_type)}), 401)
