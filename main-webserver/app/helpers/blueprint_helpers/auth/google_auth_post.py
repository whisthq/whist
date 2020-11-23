import datetime
import hashlib

from datetime import datetime as dt

from better_profanity import profanity
from flask import current_app

from app.constants.bad_words_hashed import BAD_WORDS_HASHED
from app.constants.http_codes import BAD_REQUEST, FORBIDDEN, SUCCESS
from app.helpers.utils.general.tokens import (
    generate_token,
    generate_unique_promo_code,
    get_access_tokens,
    get_google_tokens,
)
from app.models import db, User


from app.helpers.utils.datadog.events import (
    datadogEvent_userLogon,
)


def register_google_user(
    username, name, token, reason_for_signup=None
):  # pylint: disable=unused-argument
    """Registers a user, and stores it in the users table

    Args:
        username (str): The username
        name (str): The user's name (from Google)
        token (str): The generated token

    Returns:
        int: 200 on success, 400 on fail
    """
    promo_code = generate_unique_promo_code()
    username_encoding = username.lower().encode("utf-8")
    if hashlib.md5(
        username_encoding
    ).hexdigest() in BAD_WORDS_HASHED or profanity.contains_profanity(username):
        return {"status": FORBIDDEN, "error": "Try using a different username"}

    new_user = User(
        user_id=username,
        password="",
        referral_code=promo_code,
        name=name,
        reason_for_signup=reason_for_signup,
        using_google_login=True,
        created_timestamp=dt.now(datetime.timezone.utc).timestamp(),
    )

    try:
        db.session.add(new_user)
        db.session.commit()

        return {"status": SUCCESS}
    except Exception:
        return {"status": BAD_REQUEST}


def login_helper(code, client_app):
    user_obj = get_google_tokens(code, client_app)

    username, name = user_obj["email"], user_obj["name"]
    token = generate_token(username)
    access_token, refresh_token = get_access_tokens(username)

    user = User.query.get(username)

    if user:
        if user.using_google_login:
            if not current_app.testing:
                datadogEvent_userLogon(username)

            return {
                "new_user": False,
                "is_user": True,
                "token": token,
                "access_token": access_token,
                "refresh_token": refresh_token,
                "username": username,
                "status": SUCCESS,
                "name": name,
                "can_login": user.can_login,
                "using_google_login": True,
            }
        else:
            return {"status": FORBIDDEN, "error": "Try using non-Google login"}

    # if client_app:
    #     return {"status": UNAUTHORIZED, "error": "User has not registered"}

    output = register_google_user(username, name, token)

    if output["status"] == SUCCESS:
        status = 200
    else:
        status = 500

    return {
        "status": status,
        "new_user": True,
        "is_user": True,
        "token": token,
        "access_token": access_token,
        "refresh_token": refresh_token,
        "username": username,
        "name": name,
        "can_login": False,
        "using_google_login": True,
    }


def reason_helper(username, reason_for_signup):
    user = User.query.get(username)
    user.reason_for_signup = reason_for_signup

    try:
        db.session.add(user)
        db.session.commit()
        return {"status": SUCCESS}
    except Exception:
        return {"status": BAD_REQUEST}
