from app import *
from app.helpers.utils.general.tokens import *

token_bp = Blueprint("token_bp", __name__)


@token_bp.route("/token/<action>", methods=["POST"])
@jwt_refresh_token_required
@fractalPreProcess
def token(action, **kwargs):
    if action == "refresh":
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
def my_expired_token_callback(expired_token, **kwargs):
    token_type = expired_token["type"]
    return (
        jsonify({"status": 401, "msg": "The {} token has expired".format(token_type)}),
        401,
    )
