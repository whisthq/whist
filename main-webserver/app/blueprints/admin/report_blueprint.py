from app import *
from app.helpers.blueprint_helpers.admin.report_get import *
from app.helpers.blueprint_helpers.admin.report_post import *

report_bp = Blueprint("report_bp", __name__)


@report_bp.route("/report/<action>", methods=["GET"])
@fractalPreProcess
@jwt_required
@adminRequired
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


@report_bp.route("/report/regionReport", methods=["POST"])
@fractalPreProcess
@jwt_required
@adminRequired
def regionReport(**kwargs):
    body = request.get_json()
    output = regionReportHelper(body["timescale"])

    return jsonify(output), SUCCESS


@report_bp.route("/report/userReport", methods=["POST"])
@fractalPreProcess
@jwt_required
@fractalAuth
def userReport(**kwargs):
    body = request.get_json()
    output = {}
    if body["timescale"]:
        output = userReportHelper(body["username"], timescale=body["timescale"])
    elif body["start_date"]:
        output = userReportHelper(body["username"], start_date=body["start_date"])
    else:
        return jsonify({}), BAD_REQUEST

    return jsonify(output), SUCCESS
