import logging

from flask import render_template
from sendgrid import SendGridAPIClient
from sendgrid.helpers.mail import Mail

from app.constants.config import SENDGRID_API_KEY
from app.helpers.utils.general.logs import fractalLog


def signupMail(username, promo_code):
    try:
        message = Mail(
            from_email="phil@tryfractal.com",
            to_emails=username,
            subject="Welcome to Fractal",
            html_content=render_template("on_signup.html", code=promo_code),
        )
        sg = SendGridAPIClient(SENDGRID_API_KEY)

        sg.send(message)
    except Exception as e:
        fractalLog(
            function="signupMail",
            label="ERROR",
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )
