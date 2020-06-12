from app import *
from app.helpers.blueprint_helpers.account_get import *
from app.helpers.blueprint_helpers.account_post import *

account_bp = Blueprint("account_bp", __name__)


@account_bp.route("/account/<action>", methods=["POST"])
@fractalPreProcess
def account_no_auth(action, **kwargs):
    print(kwargs)
    if action == "login":
        # Login endpoint

        username, password = kwargs["body"]["username"], kwargs["body"]["password"]
        return jsonify(loginHelper(username, password)), 200

    elif action == "register":
        # Account creation endpoint

        username, password = kwargs["body"]["username"], kwargs["body"]["password"]
        name = kwargs["body"]["name"]
        reason_for_signup = kwargs["body"]["feedback"]

        return jsonify(registerHelper(username, password, name, reason_for_signup)), 200
