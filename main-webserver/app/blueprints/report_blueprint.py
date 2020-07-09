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
    elif action == "fetchLoginActivity":
        output = loginActivityHelper()
        return jsonify(output), SUCCESS
    elif action == "fetchUsers":
        output = fetchUsersHelper()
        return jsonify(output), SUCCESS
    elif action == "fetchVMs":
        output = fetchVMsHelper()
        return jsonify(output), SUCCESS
    elif action == "fetchCustomers":
        output = fetchCustomersHelper()
        return jsonify(output), SUCCESS
    elif action == "fetchDisks":
        output = fetchDisksHelper()
        return jsonify(output), SUCCESS


@report_bp.route("/report/<action>", methods=["POST"])
@fractalPreProcess
def report_post(action, **kwargs):
    if action == "regionReport":
        body = request.get_json()
        output = regionReportHelper(body["timescale"])

        return jsonify(output), SUCCESS
    elif action == "userReport":
        body = request.get_json()
        if body["timescale"]:
            output = userReportHelper(body["username"], timescale=body["timescale"])
        elif body["start_date"]:
            output = userReportHelper(body["username"], start_date=body["start_date"])
        else:
            return jsonify({}), BAD_REQUEST

        return jsonify(output), SUCCESS
