from app import *

def registerGoogleUser(username, name, token, reason_for_signup=None):
    """Registers a user, and stores it in the users table

    Args:
        username (str): The username
        name (str): The user's name (from Google)
        token (str): The generated token

    Returns:
        int: 200 on success, 400 on fail
    """
    code = genUniqueCode()

    params = {
        "userName": username,
        "password": None,
        "code": code,
        "token": token,
        "name": name,
        "reason_for_signup": reason_for_signup,
        "google_login": True,
        "verified": True,
    }
    return fractalSQLInsert("users", params)


def loginHelper(code, clientApp):
    userObj = getGoogleTokens(code, clientApp)

    username, name = userObj["email"], userObj["name"]

    token = generateToken(username)
    access_token, refresh_token = getAccessTokens(username)

    output = fractalSQLSelect(table_name="users", params={"username": username})

    if output["success"] and output["rows"]:
        if output["rows"][0]["google_login"]:
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

    if clientApp:
        return {"status": UNAUTHORIZED, "error": "User has not registered"}

    output = registerGoogleUser(username, name, token)

    if output["success"]:
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
    output = fractalSQLUpdate(
        table_name="users",
        new_params={"reason_for_signup": reason_for_signup, "verified": True},
        conditional_params={"username": username},
    )

    if output["success"]:
        return {"status": SUCCESS}
    else:
        return {"status": BAD_REQUEST}
