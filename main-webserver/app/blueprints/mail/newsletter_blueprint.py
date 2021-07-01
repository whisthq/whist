from http import HTTPStatus

from flask import Blueprint, jsonify

from app import fractal_pre_process
from app.helpers.utils.general.limiter import limiter, RATE_LIMIT_PER_MINUTE


newsletter_bp = Blueprint("newsletter_bp", __name__)


@newsletter_bp.route("/newsletter/<action>", methods=["POST"])
@limiter.limit(RATE_LIMIT_PER_MINUTE)
@fractal_pre_process
def newsletter(action, **kwargs):
    body = kwargs["body"]
    return jsonify({}), HTTPStatus.OK
