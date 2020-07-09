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
            return (
                jsonify(
                    {
                        "new_user": False,
                        "is_user": True,
                        "token": token,
                        "access_token": access_token,
                        "refresh_token": refresh_token,
                        "username": username,
                        "status": SUCCESS,
                    }
                ),
                SUCCESS,
            )
        else:
            return (
                jsonify({"status": 403, "error": "Try using non-Google login",}),
                403,
            )

    if clientApp:
        return (jsonify({"status": 401, "error": "User has not registered"}), 401)

    status = registerGoogleUser(username, name, token)

    return (
        jsonify(
            {
                "status": status,
                "new_user": True,
                "is_user": True,
                "token": token,
                "access_token": access_token,
                "refresh_token": refresh_token,
                "username": username,
            }
        ),
        status,
    )
