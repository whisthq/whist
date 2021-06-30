from typing import Dict, Union
from http import HTTPStatus

from flask import current_app

from app.exceptions import SendGridException, TemplateNotFound
from app.helpers.utils.general.logs import fractal_logger
from app.helpers.utils.mail.mail_client import MailClient


def mail_helper(
    email_id: str, from_email: str, to_email: str, email_args: Dict[str, str]
) -> Dict[str, Union[bool, int]]:
    """
    Helper function for the /mail route.

    Calls the send_email function, and catches possible exceptions.

    Parameters:
        email_id (str): ID of the email being sent (ex. EMAIL_VERIFICATION, PASSWORD_RESET,
                        etc.)
        from_email (str): email being sent from
        to_email (str): email being sent to
        email_args (dict): dict of Jinja arguments to pass into render_template()

    Returns:
        A dictionary object with the keys "status" and "verified", representing the HTTP
        response to the request.
    """

    mail_client = MailClient(current_app.config["SENDGRID_API_KEY"])

    # Don't pass keyword arguments whose values would be None.
    mail_kwargs = {
        k: v for k, v in (("from_email", from_email), ("jinja_args", email_args)) if v is not None
    }

    try:
        mail_client.send_email(email_id, to_email, **mail_kwargs)
    except (SendGridException, TemplateNotFound):
        fractal_logger.exception("Sendgrid failed to send mail")

        return {"verified": False, "status": HTTPStatus.BAD_REQUEST}

    return {"verified": True, "status": HTTPStatus.OK}
