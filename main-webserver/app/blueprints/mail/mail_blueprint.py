from flask import Blueprint
from flask import jsonify, current_app

from app import fractal_pre_process
from app.helpers.blueprint_helpers.mail.mail_post import mail_helper

mail_bp = Blueprint("mail_bp", __name__)

@mail_bp.route("/mail", methods=["POST"])
@fractal_pre_process
def mail(**kwargs):
    """
    Handles all /mail routes

    Paramters:
        **kwargs (obj): request data

    Returns:
        json: Response from API
    """

    body = kwargs.pop("body")
    args = (
        body.pop("email_id"),
        body.pop("from_email", current_app.config["SENDGRID_DEFAULT_FROM"]),
        body.pop("to_email"),
        body.pop("email_args"),
    )

    output = mail_helper(*args)
    return jsonify(output), output["status"]
