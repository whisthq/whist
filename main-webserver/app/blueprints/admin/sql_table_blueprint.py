import logging

from flask import Blueprint, jsonify, request
from flask_jwt_extended import jwt_required

from app import fractal_pre_process
from app.constants.http_codes import BAD_REQUEST, NOT_FOUND, SUCCESS
from app.helpers.blueprint_helpers.admin.report_get import fetchUsersHelper
from app.helpers.utils.general.auth import admin_required
from app.helpers.utils.general.logs import fractal_log

table_bp = Blueprint("table_bp", __name__)


@table_bp.route("/table", methods=["GET"])
@fractal_pre_process
@jwt_required
@admin_required
def sql_table_get(**kwargs):
    output = None
    table_name = request.args.get("table_name")

    if table_name == "users":
        output = fetchUsersHelper()

    if output:
        return jsonify({"output": output}), SUCCESS
    else:
        fractal_log(
            function="sql_table_get",
            label=table_name,
            logs=str(output["error"]),
            level=logging.ERROR,
        )
        if not output["rows"]:
            return jsonify({"output": None}), NOT_FOUND
        else:
            return jsonify({"output": None}), BAD_REQUEST
