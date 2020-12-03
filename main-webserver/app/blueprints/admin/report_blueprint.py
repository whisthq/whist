from flask import Blueprint, jsonify, request
from flask_jwt_extended import jwt_required

from app import fractal_pre_process
from app.constants.http_codes import BAD_REQUEST, SUCCESS
from app.helpers.blueprint_helpers.admin.report_get import (
    fetchUsersHelper,
    latestHelper,
    loginActivityHelper,
    signupsHelper,
    totalUsageHelper,
)
from app.helpers.blueprint_helpers.admin.report_post import (
    regionReportHelper,
    userReportHelper,
)
from app.helpers.utils.general.auth import admin_required, fractal_auth

report_bp = Blueprint("report_bp", __name__)


@report_bp.route("/report/<action>", methods=["GET"])
@fractal_pre_process
@jwt_required
@admin_required
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


@report_bp.route("/report/regionReport", methods=["POST"])
@fractal_pre_process
@jwt_required
@admin_required
def regionReport(**kwargs):
    body = request.get_json()
    output = regionReportHelper(body["timescale"])

    return jsonify(output), SUCCESS


@report_bp.route("/report/userReport", methods=["POST"])
@fractal_pre_process
@jwt_required
@fractal_auth
def userReport(**kwargs):
    body = request.get_json()

    if body["timescale"]:
        return userReportHelper(body["username"], timescale=body["timescale"])
    elif body["start_date"]:
        return userReportHelper(body["username"], start_date=body["start_date"])
    else:
        return jsonify({}), BAD_REQUEST
