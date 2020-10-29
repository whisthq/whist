from flask import Blueprint, jsonify, request
from flask_jwt_extended import jwt_required

from app import fractalPreProcess
from app.constants.http_codes import SUCCESS
from app.helpers.blueprint_helpers.auth.account_get import (
    codeHelper,
    verifiedHelper,
)
from app.helpers.blueprint_helpers.auth.account_post import (
    deleteHelper,
    loginHelper,
    lookupHelper,
    registerHelper,
    updateUserHelper,
    verifyHelper,
    resetPasswordHelper,
)
from app.helpers.utils.general.auth import fractalAuth

account_bp = Blueprint("account_bp", __name__)


@account_bp.route("/account/delete", methods=["POST"])
@fractalPreProcess
@jwt_required
@fractalAuth
def account_postDelete(**kwargs):
    # Account deletion endpoint
    username = kwargs["body"]["username"]

    output = deleteHelper(username)

    return jsonify(output), output["status"]


@account_bp.route("/account/update", methods=["POST"])
@fractalPreProcess
@jwt_required
@fractalAuth
def account_postUpdate(**kwargs):
    # Change the user's name, email, or password
    return updateUserHelper(kwargs["body"])


@account_bp.route("/account/<action>", methods=["POST"])
@fractalPreProcess
def account_post(action, **kwargs):
    body = kwargs["body"]
    if action == "login":
        # Login endpoint

        username, password = body["username"], body["password"]

        output = loginHelper(username, password)

        return jsonify(output), SUCCESS

    elif action == "register":
        # Account creation endpoint

        username, password = body["username"], body["password"]
        name = body["name"]
        reason_for_signup = body["feedback"]

        output = registerHelper(username, password, name, reason_for_signup)

        return jsonify(output), output["status"]

    elif action == "verify":
        # Email verification endpoint

        username, token = body["username"], body["token"]

        output = verifyHelper(username, token)

        return jsonify(output), output["status"]
    elif action == "lookup":
        # Check if user exists

        output = lookupHelper(body["username"])
        return jsonify(output), output["status"]


@account_bp.route("/account/<action>", methods=["GET"])
@fractalPreProcess
def account_get_no_auth(action, **kwargs):
    if action == "verified":
        # Check if the user's email has been verified
        username = request.args.get("username")

        output = verifiedHelper(username)

        return jsonify(output), output["status"]

    # TODO: Delete later
    elif action == "code":
        # Get the user's promo code
        username = request.args.get("username")

        output = codeHelper(username)

        return jsonify(output), output["status"]
