from flask import jsonify
from flask_jwt_extended import decode_token
from flask_jwt_extended import get_jwt_identity

from app.constants.http_codes import SUCCESS, UNAUTHORIZED
from app.helpers.blueprint_helpers.auth.account_get import fetch_user_helper


def validate_token_helper():
    current_user = get_jwt_identity()
    output = fetch_user_helper(current_user)
    return output
