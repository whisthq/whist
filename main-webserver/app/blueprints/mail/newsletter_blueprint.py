from flask import Blueprint, jsonify

from app import fractal_pre_process
from app.constants.http_codes import SUCCESS
from app.helpers.utils.general.limiter import limiter, LIMIT


newsletter_bp = Blueprint("newsletter_bp", __name__)


@newsletter_bp.route("/newsletter/<action>", methods=["POST"])
@limiter.limit(LIMIT)
@fractal_pre_process
def newsletter(action, **kwargs):
    body = kwargs["body"]
    return jsonify({}), SUCCESS
