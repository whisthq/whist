from flask import Blueprint

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
        body.pop("from_email"),
        body.pop("to_emails"),
        body.pop("email_args")
    )

    mail_helper(*args)
