import json

from flask import current_app
from jose import jwt
from flask_jwt_extended import get_jwt_identity

from app.models import User
from app.helpers.utils.general.logs import fractal_logger


def auth_token_helper(auth_token):
    """A helper function that processes any tokens passed to the /hasura/auth
    endpoint through the Authorization header

    Args:
        auth_token: A string that is either empty or contains "Bearer some_auth_token"
    Returns:
        A dictionary containing key value pairs for "X-Hasura-Role" and "X-Hasura-User-Id"
    """
    hasura_role = {"X-Hasura-Role": "anonymous", "X-Hasura-User-Id": ""}

    if auth_token:
        auth_token = auth_token.replace("Bearer ", "")
        current_user = ""

        username = get_jwt_identity()
        if username:
            hasura_role = {"X-Hasura-Role": "user", "X-Hasura-User-Id": current_user}
        # original code:
        # try:
        #     decoded_key = jwt.decode(auth_token, current_app.config["JWT_SECRET_KEY"])
        #     if decoded_key:
        #         current_user = decoded_key["identity"]
        #         user = None if not current_user else User.query.get(current_user)
        #         if user and current_user:
        #             hasura_role = {"X-Hasura-Role": "user", "X-Hasura-User-Id": current_user}
        # except Exception:
        #     pass

    return hasura_role


def login_token_helper(login_token):
    """A helper function that processes any tokens passed to the /hasura/auth endpoint
    through the Login header

    Args:
        login: A string that is either empty or contains a login token generated from the
        client-application
    Returns:
        A dictionary containing a single key "X-Hasura-Login-Token".
    """
    hasura_login = {"X-Hasura-Login-Token": ""}
    if login_token:
        hasura_login = {"X-Hasura-Login-Token": login_token}

    return hasura_login
