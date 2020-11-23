import json

from flask import Blueprint, request
from flask_jwt_extended import jwt_required

from app import fractal_pre_process
from app.helpers.blueprint_helpers.admin.analytics_post import analyticsLogsHelper
from app.helpers.utils.general.auth import admin_required

analytics_bp = Blueprint("analytics_bp", __name__)


@analytics_bp.route("/analytics/<action>", methods=["POST"])
@fractal_pre_process
@jwt_required
@admin_required
def analytics_post(action, **kwargs):
    if action == "logs":
        body = json.loads(request.data)

        return analyticsLogsHelper(body)
