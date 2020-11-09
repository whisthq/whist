from flask import Blueprint, jsonify
from flask_jwt_extended import jwt_required

from app import fractalPreProcess
from app.constants.http_codes import SUCCESS

from app.helpers.blueprint_helpers.admin.hasura_get import authHelper

hasura_bp = Blueprint("hasura_bp", __name__)


@hasura_bp.route("/hasura/auth", methods=["GET"])
@fractalPreProcess
@jwt_required
def hasura_auth_get(**kwargs):
    output = authHelper()
    return jsonify(output), SUCCESS
