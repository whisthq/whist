from flask import Blueprint, jsonify, request
from flask_jwt_extended import jwt_required

from app import fractal_pre_process
from app.constants.http_codes import SUCCESS
from app.helpers.blueprint_helpers.auth.account_get import (
    code_helper,
    verified_helper,
)
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

account_bp = Blueprint("account_bp", __name__)


@account_bp.route("/account/delete", methods=["POST"])
@fractal_pre_process
@jwt_required
@fractal_auth
def account_post_delete(**kwargs):
    # Account deletion endpoint
    username = kwargs["body"]["username"]

    output = delete_helper(username)

    return jsonify(output), output["status"]


@account_bp.route("/account/update", methods=["POST"])
@fractal_pre_process
@jwt_required
@fractal_auth
def account_post_update(**kwargs):
    # Change the user's name, email, or password
    return update_user_helper(kwargs["body"])


@account_bp.route("/account/<action>", methods=["POST"])
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

        username, password = body["username"], body["password"]
        name = body["name"]
        reason_for_signup = body["feedback"]
        can_login = body.pop("can_login", False)

        output = register_helper(username, password, name, reason_for_signup, can_login)

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


@account_bp.route("/account/<action>", methods=["GET"])
@fractal_pre_process
def account_get_no_auth(action, **kwargs):  # pylint: disable=unused-argument
    if action == "verified":
        # Check if the user's email has been verified
        username = request.args.get("username")

        output = verified_helper(username)

        return jsonify(output), output["status"]

    # TODO: Delete later
    elif action == "code":
        # Get the user's promo code
        username = request.args.get("username")

        output = code_helper(username)

        return jsonify(output), output["status"]
