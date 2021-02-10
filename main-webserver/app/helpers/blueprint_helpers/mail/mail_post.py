from flask import current_app

from app.constants.http_codes import SUCCESS, BAD_REQUEST
from app.helpers.utils.mail.mail_client import MailClient, SendGridException


def mail_helper(email_id, from_email, to_email, email_args):
    try:
        mail_client = MailClient(current_app.config["SENDGRID_API_KEY"])
        mail_client.send_email(
            from_email=from_email, to_email=to_email, email_id=email_id, jinja_args=email_args
        )
        return {"status": SUCCESS}
    except SendGridException:
        return {"status": BAD_REQUEST}
