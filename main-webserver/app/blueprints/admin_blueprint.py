from app import *
from app.helpers.blueprint_helpers.admin_post import *

admin_bp = Blueprint("admin_bp", __name__)


@admin_bp.route("/admin/<action>", methods=["POST"])
@fractalPreProcess
def admin_no_auth(action, **kwargs):
    if action == "login":
        body = request.get_json()
        output = None
        if body["username"] and body["password"]:
            output = adminLoginHelper(body["username"], body["password"])

        if output:
            return jsonify(output), SUCCESS
        else:
            return jsonify({}), UNAUTHORIZED
