from flask import Blueprint
from flask.json import jsonify
from flask_jwt_extended import jwt_required

from app import log_request

from app.models import RegionToAmi

aws_region_bp = Blueprint("aws_region_bp", __name__)


@aws_region_bp.route("/regions", methods=("GET",))
@log_request
@jwt_required()
def regions():
    """Return the list of regions in which users are allowed to deploy tasks.

    Returns:
        A list of strings, where each string is the name of a region.
    """

    enabled_regions = RegionToAmi.query.filter_by(ami_active=True).distinct(RegionToAmi.region_name)

    return jsonify([region.region_name for region in enabled_regions])
