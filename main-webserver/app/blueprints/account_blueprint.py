from app import *
from app.helpers.blueprint_helpers.account_get import *
from app.helpers.blueprint_helpers.account_post import *

account_bp = Blueprint("account_bp", __name__)

<<<<<<< HEAD

@account_bp.route("/account/<action>", methods=["POST"])
@fractalPreProcess
def account_no_auth(action, **kwargs):
    if action == "login":
        # Login endpoint

        username, password = kwargs["body"]["username"], kwargs["body"]["password"]

        output = loginHelper(username, password)

        return jsonify(output), SUCCESS

    elif action == "register":
        # Account creation endpoint

        username, password = kwargs["body"]["username"], kwargs["body"]["password"]
        name = kwargs["body"]["name"]
        reason_for_signup = kwargs["body"]["feedback"]

        output = registerHelper(username, password, name, reason_for_signup)

        return jsonify(output), output["status"]

    elif action == "verify":
        # Email verification endpoint

        username, token = kwargs["body"]["username"], kwargs["body"]["token"]

        output = verifyHelper(username, token)

        return jsonify(output), output["status"]

    elif action == "delete":
        # Account deletion endpoint

        username = kwargs["body"]["username"]

        output = deleteHelper(username)

        return jsonify(output), output["status"]


@account_bp.route("/account/<action>", methods=["GET"])
@fractalPreProcess
def account_get(action, **kwargs):
    if action == "code":
        # Get the user's promo code

        username = request.args.get("username")

        output = codeHelper(username)

        return jsonify(output), output["status"]

    elif action == "disks":
        # Get all the user's disks

        username = request.args.get("username")

        main = True
        if "main" in request.args:
            main = str(request.args.get("main")).upper() == "TRUE"

        output = disksHelper(username, main)

        return jsonify(output), output["status"]

    elif action == "verified":
        # Check if the user's email has been verified

        username = request.args.get("username")

        output = verifiedHelper(username)

        return jsonify(output), output["status"]
=======
# TO be deleted
@account_bp.route("/user/login", methods=["POST"])
@generateID
@logRequestInfo
def user_login(**kwargs):
    body = request.get_json()
    username = body["username"]
    payload = fetchUserDisks(username, ID=kwargs["ID"])
    return jsonify({"payload": payload}), 200


@account_bp.route("/user/fetchvms", methods=["POST"])
@jwt_required
@generateID
@logRequestInfo
def user_fetchvms(**kwargs):
    body = request.get_json()
    username = body["username"]
    try:
        return jsonify({"vms": fetchUserVMs(username, kwargs["ID"])}), 200
    except Exception as e:
        return jsonify({}), 403


@account_bp.route("/user/fetchdisks", methods=["POST"])
@generateID
@logRequestInfo
def user_fetchdisks(**kwargs):
    body = request.get_json()

    main = True
    if "main" in body.keys():
        main = body["main"]

    disks = fetchUserDisks(body["username"], False, main=main, ID=kwargs["ID"])
    return jsonify({"disks": disks, "status": 200}), 200


@account_bp.route("/user/reset", methods=["POST"])
@jwt_required
@generateID
@logRequestInfo
def user_reset(**kwargs):
    body = request.get_json()
    username, vm_name = body["username"], body["vm_name"]
    resetVMCredentials(username, vm_name)
    return jsonify({"status": 200}), 200


@account_bp.route("/account/googleLogin", methods=["POST"])
@generateID
@logRequestInfo
def google_login(**kwargs):
    body = request.get_json()
    code = body["code"]

    clientApp = False
    if "clientApp" in body.keys():
        clientApp = body["clientApp"]

    userObj = getGoogleTokens(code, clientApp)

    sendInfo(kwargs["ID"], userObj)

    username, name = userObj["email"], userObj["name"]

    token = generateToken(username)
    access_token, refresh_token = getAccessTokens(username)

    if lookup(username):
        if isGoogle(username):
            # User logging in again with Google
            vm_status = userVMStatus(username)

            return (
                jsonify(
                    {
                        "new_user": False,
                        "is_user": True,
                        "vm_status": vm_status,
                        "token": token,
                        "access_token": access_token,
                        "refresh_token": refresh_token,
                        "username": username,
                        "status": 200,
                    }
                ),
                200,
            )
        else:
            return (
                jsonify({"status": 403, "error": "Try using non-Google login",}),
                403,
            )

    if clientApp:
        return (jsonify({"status": 401, "error": "User has not registered"}), 401)

    sendInfo(kwargs["ID"], "Registering a new user with Google")
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


@account_bp.route("/account/googleReason", methods=["POST"])
@generateID
@logRequestInfo
def google_reason(**kwargs):
    print(request)
    body = request.get_json()

    username, reason_for_signup = body["username"], body["reason"]
    setUserReason(username, reason_for_signup)
    makeUserVerified(username, True)

    return (jsonify({"status": 200}), 200)


# When people log into their account
@account_bp.route("/account/login", methods=["POST"])
@generateID
@logRequestInfo
def account_login(**kwargs):
    body = request.get_json()
    username, password = body["username"], body["password"]

    if lookup(username) and isGoogle(username):
        return (
            jsonify({"error": "Try using Google login instead", "status": 403}),
            403,
        )

    is_user = password != os.getenv("ADMIN_PASSWORD")
    verified = loginUser(username, password)
    vm_status = userVMStatus(username)
    token = fetchUserToken(username)
    access_token, refresh_token = getAccessTokens(username)
    return (
        jsonify(
            {
                "verified": verified,
                "is_user": is_user,
                "vm_status": vm_status,
                "token": token,
                "access_token": access_token,
                "refresh_token": refresh_token,
            }
        ),
        200,
    )


@account_bp.route("/account/register", methods=["POST"])
@generateID
@logRequestInfo
def account_register(**kwargs):
    body = request.get_json()
    username, password = body["username"], body["password"]

    name = None
    if "name" in body.keys():
        name = body["name"]

    reason_for_signup = None
    if "feedback" in body.keys():
        reason_for_signup = body["feedback"]

    sendInfo(kwargs["ID"], "Registering a new user")
    token = generateToken(username)
    status = registerUser(username, password, token, name, reason_for_signup)
    access_token, refresh_token = getAccessTokens(username)
    return (
        jsonify(
            {
                "status": status,
                "token": token,
                "access_token": access_token,
                "refresh_token": refresh_token,
            }
        ),
        status,
    )


@account_bp.route("/account/lookup", methods=["POST"])
@generateID
@logRequestInfo
def account_lookup(**kwargs):
    body = request.get_json()
    username = body["username"]

    return jsonify({"exists": lookup(username)}), 200


@account_bp.route("/account/checkVerified", methods=["POST"])
@generateID
@logRequestInfo
def account_check_verified(**kwargs):
    body = request.get_json()
    username = body["username"]
    sendInfo(kwargs["ID"], "Checking if user {} is verified".format(username))
    verified = checkUserVerified(username)
    return jsonify({"status": 200, "verified": verified}), 200


@account_bp.route("/account/verifyUser", methods=["POST"])
@jwt_required
@generateID
@logRequestInfo
def account_verify_user(**kwargs):
    body = request.get_json()
    username = body["username"]
    correct_token = fetchUserToken(username)
    if body["token"] == correct_token:
        sendInfo(kwargs["ID"], "User {} is now verified".format(username))
        makeUserVerified(username, True)
        return jsonify({"status": 200, "verified": True}), 200
    else:
        sendInfo(kwargs["ID"], "User {} cannot be verified".format(username))
        return jsonify({"status": 401, "verified": False}), 401


@account_bp.route("/account/generateIDs", methods=["POST"])
@jwt_required
@generateID
@logRequestInfo
def account_generate_ids(**kwargs):
    result = generateIDs(kwargs["ID"])
    return jsonify({"status": result}), result


@account_bp.route("/account/fetchUsers", methods=["POST"])
@jwt_required
@generateID
@logRequestInfo
def account_fetch_users(**kwargs):
    users = fetchAllUsers()
    return jsonify({"status": 200, "users": users}), 200


@account_bp.route("/account/fetchUser", methods=["POST"])
@jwt_required
@generateID
@logRequestInfo
def account_fetch_user(**kwargs):
    body = request.get_json()
    user = fetchUser(body["username"])
    return jsonify({"user": user}), 200


@account_bp.route("/account/delete", methods=["POST"])
@jwt_required
@generateID
@logRequestInfo
def account_delete(**kwargs):
    body = request.get_json()
    status = deleteUser(body["username"])
    return jsonify({"status": status}), status


@account_bp.route("/account/fetchCode", methods=["POST"])
@generateID
@logRequestInfo
def account_fetch_code(**kwargs):
    body = request.get_json()
    code = fetchUserCode(body["username"])
    if code:
        return jsonify({"status": 200, "code": code}), 200
    else:
        return jsonify({"status": 404, "code": None}), 404


@account_bp.route("/account/feedback", methods=["POST"])
@jwt_required
@generateID
@logRequestInfo
def account_feedback(**kwargs):
    body = request.get_json()
    storeFeedback(body["username"], body["feedback"])
    return jsonify({"status": 200}), 200


@account_bp.route("/admin/<action>", methods=["POST"])
@generateID
@logRequestInfo
def admin(action, **kwargs):
    body = request.get_json()
    if action == "login":
        if body["username"] == os.getenv("DASHBOARD_USERNAME") and body[
            "password"
        ] == os.getenv("DASHBOARD_PASSWORD"):
            access_token, refresh_token = getAccessTokens(body["username"])
            sendInfo(kwargs["ID"], "Admin login successful")
            return (
                jsonify(
                    {
                        "status": 200,
                        "access_token": access_token,
                        "refresh_token": refresh_token,
                    }
                ),
                200,
            )
        sendInfo(kwargs["ID"], "Admin login unsuccessful")
        return jsonify({"status": 422}), 422


# TOKEN endpoint (for access tokens)
# TODO: work for Google
@account_bp.route("/token/refresh", methods=["POST"])
@jwt_refresh_token_required
@generateID
@logRequestInfo
def token_refresh(**kwargs):
    username = get_jwt_identity()
    access_token, refresh_token = getAccessTokens(username)
    return (
        jsonify(
            {
                "status": 200,
                "username": username,
                "access_token": access_token,
                "refresh_token": refresh_token,
            }
        ),
        200,
    )


@jwtManager.expired_token_loader
@generateID
@logRequestInfo
def my_expired_token_callback(expired_token, **kwargs):
    token_type = expired_token["type"]
    return (
        jsonify({"status": 401, "msg": "The {} token has expired".format(token_type)}),
        401,
    )
>>>>>>> staging
