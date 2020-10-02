from app import *
from app.helpers.utils.general.tokens import *
from app.constants.bad_words_hashed import BAD_WORDS_HASHED
from app.constants.generate_subsequences_for_words import generate_subsequence_for_word
from app.models.public import *
from app.models.hardware import *
from app.serializers.public import *
from app.serializers.hardware import *


def registerGoogleUser(username, name, token, reason_for_signup=None):
    """Registers a user, and stores it in the users table

    Args:
        username (str): The username
        name (str): The user's name (from Google)
        token (str): The generated token

    Returns:
        int: 200 on success, 400 on fail
    """
    promo_code = generateUniquePromoCode()
    username_subsq = generate_subsequence_for_word(username)
    for result in username_subsq:
        username_encoding = result.lower().encode("utf-8")
        if hashlib.md5(username_encoding).hexdigest() in BAD_WORDS_HASHED:
            return {"status": FAILURE, "error": "Try using a different username"}

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


def loginHelper(code, clientApp):
    userObj = getGoogleTokens(code, clientApp)

    username, name = userObj["email"], userObj["name"]
    token = generateToken(username)
    access_token, refresh_token = getAccessTokens(username)

    user = User.query.get(username)

    if user:
        if user.using_google_login:
            return {
                "new_user": False,
                "is_user": True,
                "token": token,
                "access_token": access_token,
                "refresh_token": refresh_token,
                "username": username,
                "status": SUCCESS,
            }
        else:
            return {"status": FORBIDDEN, "error": "Try using non-Google login"}

    # if clientApp:
    #     return {"status": UNAUTHORIZED, "error": "User has not registered"}

    output = registerGoogleUser(username, name, token)

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
    }


def reasonHelper(username, reason_for_signup):
    user = User.query.get(username)
    user.reason_for_signup = reason_for_signup

    try:
        db.session.add(user)
        db.session.commit()
        return {"status": SUCCESS}
    except Exception:
        return {"status": BAD_REQUEST}
