from typing import Dict, Union, Optional

from flask_jwt_extended import get_jwt_identity

from app.helpers.blueprint_helpers.auth.account_get import fetch_user_helper


def validate_token_helper() -> Dict[str, Union[Optional[str], int]]:
    """
    Confirms the bearer token sent by the client app and returns the matching user object
    Returns: a json containing user information

    """
    current_user = get_jwt_identity()
    output = fetch_user_helper(current_user)
    return output
