from flask import jsonify

from app.constants.http_codes import BAD_REQUEST, SUCCESS
from app.models import User
from app.serializers.public import UserSchema
from app.helpers.utils.general.tokens import getAccessTokens

user_schema = UserSchema()


def codeHelper(username):
    """Fetches a user's promo code

    Parameters:
    username (str): The username

    Returns:
    json: The user's promo code
    """

    # Query database for user

    user = User.query.get(username)

    # Return user's promo code

    if user:
        code = user.referral_code

        return {"code": code, "status": SUCCESS}

    else:
        return {"code": None, "status": BAD_REQUEST}


def fetchUserHelper(username):
    """Returns the user's entire info

    Args:
        username (str): Email of the user

    Returns:
        http response
    """
    user = User.query.get(username)

    # Return user
    if user:
        access_token, refresh_token = getAccessTokens(username)
        output = user_schema.dump(user)
        output["access_token"] = access_token
        output["refresh_token"] = refresh_token 
        return {"user": output, "status": SUCCESS}
    else:
        return {"user": None, "status": BAD_REQUEST}


def verifiedHelper(username):
    """Checks if a user's email has been verified

    Parameters:
    username (str): The username

    Returns:
    json: Whether the email is verified
    """

    # Query database for user

    user = User.query.get(username)

    if not user:
        return {"verified": False, "status": SUCCESS}
    else:
        return {"verified": user.verified, "status": SUCCESS}
