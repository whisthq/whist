from app import *
from app.helpers.blueprint_helpers.report_get import *
from app.helpers.blueprint_helpers.report_post import *

report_bp = Blueprint("report_bp", __name__)


@report_bp.route("/report/<action>", methods=["GET"])
@fractalPreProcess
def report_get(action, **kwargs):
    if action == "latest":
        output = latestHelper()

        if output:
            return jsonify(output), SUCCESS
        else:
            return jsonify({}), BAD_REQUEST

    elif action == "totalUsage":
        output = totalUsageHelper()

        return jsonify(output), SUCCESS
    elif action == "signups":
        output = signupsHelper()

        return jsonify(output), SUCCESS

@report_bp.route("/report/<action>", methods=["POST"])
@fractalPreProcess
def report_post(action, **kwargs):
    if action == "regionReport":
        body = request.get_json()
        output = regionReportHelper(body["timescale"])

        return jsonify(output), SUCCESS

@report_bp.route("/report/<action>", methods=["POST"])
@fractalPreProcess
def report_no_admin(action, **kwargs):
    if action == "userReport":
        return
