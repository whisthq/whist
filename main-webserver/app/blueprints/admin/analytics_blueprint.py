import json

from flask import Blueprint, request
from flask_jwt_extended import jwt_required

from app import fractalPreProcess
from app.helpers.blueprint_helpers.admin.analytics_post import (
    analyticsLogsHelper,
)
from app.helpers.utils.general.auth import adminRequired

analytics_bp = Blueprint("analytics_bp", __name__)


@analytics_bp.route("/analytics/<action>", methods=["POST"])
@fractalPreProcess
@jwt_required
@adminRequired
def analytics_post(action, **kwargs):
    if action == "logs":
        body = json.loads(request.data)

        return analyticsLogsHelper(body)
