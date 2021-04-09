from typing import Dict
from flask_jwt_extended import get_jwt_identity


def auth_token_helper(auth_token: str) -> Dict[str, str]:
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

        username = get_jwt_identity()
        if username:
            hasura_role = {"X-Hasura-Role": "user", "X-Hasura-User-Id": username}

    return hasura_role


def login_token_helper(login_token: str) -> Dict[str, str]:
    """A helper function that processes any tokens passed to the /hasura/auth endpoint
    through the Login header

    Args:
        login_token: A string that is either empty or contains a login token generated from the
        client-application
    Returns:
        A dictionary containing a single key "X-Hasura-Login-Token".
    """
    hasura_login = {"X-Hasura-Login-Token": ""}
    if login_token:
        hasura_login = {"X-Hasura-Login-Token": login_token}

    return hasura_login
