from app import *


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

    status = registerGoogleUser(username, name, token)

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
    return fractalSQLUpdate(
        table_name="users",
        new_params={"reason_for_signup": reason_for_signup, "verified": True},
        conditional_params={"username": username},
    )
