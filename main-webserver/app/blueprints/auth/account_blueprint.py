from flask import Blueprint, jsonify
from flask_jwt_extended import jwt_required

from app import fractal_pre_process
from app.constants.http_codes import SUCCESS
from app.helpers.blueprint_helpers.auth.account_post import (
    delete_helper,
    login_helper,
    lookup_helper,
    register_helper,
    update_user_helper,
    verify_helper,
    auto_login_helper,
    verify_password_helper,
)
from app.helpers.utils.general.auth import fractal_auth
from app.helpers.utils.general.limiter import limiter, RATE_LIMIT_PER_MINUTE

account_bp = Blueprint("account_bp", __name__)


@account_bp.route("/account/delete", methods=["POST"])
@limiter.limit(RATE_LIMIT_PER_MINUTE)
@fractal_pre_process
@jwt_required()
@fractal_auth
def account_post_delete(**kwargs):
    # Account deletion endpoint
    username = kwargs["body"]["username"]

    output = delete_helper(username)

    return jsonify(output), output["status"]


@account_bp.route("/account/update", methods=["POST"])
@limiter.limit(RATE_LIMIT_PER_MINUTE)
@fractal_pre_process
@jwt_required()
@fractal_auth
def account_post_update(**kwargs):
    # Change the user's name, email, or password
    return update_user_helper(kwargs["body"])


@account_bp.route("/account/<action>", methods=["POST"])
@limiter.limit(RATE_LIMIT_PER_MINUTE)
@fractal_pre_process
def account_post(action, **kwargs):
    body = kwargs["body"]
    if action == "login":
        # Login endpoint

        username, password = body["username"], body["password"]

        output = login_helper(username, password)

        return jsonify(output), SUCCESS

    elif action == "register":
        # Account creation endpoint
        # Only returns email verification, access, and refresh tokens if
        # the username ends in @fractal.co for testing frontend integration tests

        username, password, encrypted_config_token = (
            body["username"],
            body["password"],
            body["encrypted_config_token"],
        )
        name = body["name"]
        reason_for_signup = body["feedback"]

        output = register_helper(
            username, password, encrypted_config_token, name, reason_for_signup
        )

        return jsonify(output), output["status"]

    elif action == "verify":
        # Email verification endpoint

        username, token = body["username"], body["token"]

        output = verify_helper(username, token)

        return jsonify(output), output["status"]

    elif action == "lookup":
        # Check if user exists

        output = lookup_helper(body["username"])
        return jsonify(output), output["status"]

    elif action == "auto_login":
        # Automatic login endpoint if user has selected Remember Me

        username = body["username"]

        output = auto_login_helper(username)

        return jsonify(output), output["status"]

    elif action == "verify_password":
        # Verifies correct current password before allowing a user to change password

        username, password = body["username"], body["password"]

        output = verify_password_helper(username, password)

        return jsonify(output), output["status"]
