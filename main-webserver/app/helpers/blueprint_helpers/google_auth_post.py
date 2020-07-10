from app import *


def loginHelper(code, clientApp):
    clientApp = False

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
