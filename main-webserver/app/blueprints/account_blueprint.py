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
        return jsonify(loginHelper(username, password)), 200

    elif action == "register":
        # Account creation endpoint

        username, password = kwargs["body"]["username"], kwargs["body"]["password"]
        name = kwargs["body"]["name"]
        reason_for_signup = kwargs["body"]["feedback"]

        output = registerHelper(username, password, name, reason_for_signup)

        return jsonify(output), output["status"]

    elif action == "reset":
        print("Hello world")


@account_bp.route("/account/<action>", methods=["GET"])
@fractalPreProcess
def account_get(action, **kwargs):
    if action == "disks":
        username = request.args.get("username")

        main = True
        if "main" in request.args:
            main = str(request.args.get("main")).upper() == "TRUE"

        output = disksHelper(username, main)

        return jsonify(output), output["status"]
