from app import *
from app.helpers.feedback import *
from app.helpers.vms import *
from app.helpers.users import *
from app.helpers.disks import *

from app.logger import *

account_bp = Blueprint("account_bp", __name__)
logger = logging.getLogger("root")

# TO be deleted
@account_bp.route("/user/login", methods=["POST"])
@generateID
@logRequestInfo
def user_login(**kwargs):
    body = request.get_json()
    username = body["username"]
    payload = fetchUserDisks(username, ID=kwargs["ID"])
    sendInfo(-1, "testInfo")
    sendError(-1, "testError")
    sendWarning(-1, "testWarning")

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


# When people log into their account
@account_bp.route("/account/login", methods=["POST"])
@generateID
@logRequestInfo
def account_login(**kwargs):
    body = request.get_json()
    username, password = body["username"], body["password"]
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


@account_bp.route("/account/checkVerified", methods=["POST"])
@generateID
@logRequestInfo
def account_check_verified(**kwargs):
    body = request.get_json()
    username = body["username"]
    sendInfo(kwargs["ID"], "Checking if user {} is verified".format(username))
    verified = checkUserVerified(username)
    return jsonify({"status": 200, "verified": verified}), 200


@account_bp.route("/account/lookup", methods=["POST"])
@generateID
@logRequestInfo
def account_lookup(**kwargs):
    body = request.get_json()
    username = body["username"]

    return jsonify({"exists": lookup(username)}), 200


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
