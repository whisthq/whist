from app import *
from app.helpers.blueprint_helpers.account_get import *
from app.helpers.blueprint_helpers.account_post import *

account_bp = Blueprint("account_bp", __name__)


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
@jwt_required
def account_get(action, **kwargs):
    username = request.args.get("username")
    current_user = get_jwt_identity()
    if current_user != username:
        return  jsonify({ "error": "Wrong user!"}), UNAUTHORIZED

    if action == "code":
        # Get the user's promo code

        output = codeHelper(username)

        return jsonify(output), output["status"]

    elif action == "disks":
        # Get all the user's disks

        main = True
        if "main" in request.args:
            main = str(request.args.get("main")).upper() == "TRUE"

        output = disksHelper(username, main)

        return jsonify(output), output["status"]

    elif action == "verified":
        # Check if the user's email has been verified

        output = verifiedHelper(username)

        return jsonify(output), output["status"]
