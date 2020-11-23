from flask import Blueprint, jsonify
from flask_jwt_extended import get_jwt_identity, jwt_refresh_token_required

from app import fractalPreProcess, jwtManager
from app.helpers.blueprint_helpers.auth.token_post import validateTokenHelper
from app.helpers.utils.general.tokens import getAccessTokens

token_bp = Blueprint("token_bp", __name__)


@token_bp.route("/token/refresh", methods=["POST"])
@jwt_refresh_token_required
@fractalPreProcess
def token(**kwargs):  # pylint: disable=unused-argument
    username = get_jwt_identity()
    access_token, refresh_token = getAccessTokens(username)

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


@token_bp.route("/token/validate", methods=["POST"])
@fractalPreProcess
def validateToken(**kwargs):
    body = kwargs["body"]
    return validateTokenHelper(body["token"])


@jwtManager.expired_token_loader
@fractalPreProcess
def my_expired_token_callback(expired_token, **kwargs):  # pylint: disable=unused-argument
    token_type = expired_token["type"]
    return (jsonify({"status": 401, "msg": "The {} token has expired".format(token_type)}), 401)
