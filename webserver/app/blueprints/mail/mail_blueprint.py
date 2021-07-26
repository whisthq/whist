from flask import Blueprint
from flask import jsonify, current_app
from flask_jwt_extended import jwt_required

from app import fractal_pre_process
from app.helpers.blueprint_helpers.mail.mail_post import mail_helper
from app.helpers.utils.general.auth import developer_required

mail_bp = Blueprint("mail_bp", __name__)


@mail_bp.route("/mail", methods=["POST"])
@fractal_pre_process
@jwt_required()
@developer_required
def mail(**kwargs):
    """
    Handles all /mail routes. Needs:
    - email_id (str): email_id of the mail template to be used
    - from_email (str): email to send from (by default, "noreply@fractal.co")
    - to_email (str): email to send to
    - email_args (dict): dictionary containing arguments to fill into the email templates
        (i.e. personalized name, link to rest password, etc.)

    Paramters:
        **kwargs (obj): request data

    Returns:
        json: A dictionary object with the keys "status" and "verified", representing
        the HTTP response to the mail request.
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
