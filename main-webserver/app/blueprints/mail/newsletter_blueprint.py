from flask import Blueprint, jsonify

from app import fractalPreProcess
from app.constants.http_codes import SUCCESS

newsletter_bp = Blueprint("newsletter_bp", __name__)


@newsletter_bp.route("/newsletter/<action>", methods=["POST"])
@fractalPreProcess
def newsletter(action, **kwargs):
    body = kwargs["body"]
    return jsonify({}), SUCCESS
