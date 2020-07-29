from app import *
from app.helpers.blueprint_helpers.admin.admin_post import *

admin_bp = Blueprint("admin_bp", __name__)


@admin_bp.route("/admin/<action>", methods=["POST"])
@fractalPreProcess
def admin_post(action, **kwargs):
    body = kwargs["body"]
    if action == "login":
        output = None
        if body["username"] and body["password"]:
            output = adminLoginHelper(body["username"], body["password"])

        if output:
            return jsonify(output), SUCCESS
        else:
            return jsonify({}), UNAUTHORIZED
