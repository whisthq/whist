import logging

from flask import current_app, render_template
from sendgrid import SendGridAPIClient
from sendgrid.helpers.mail import Mail

from app.helpers.utils.general.logs import fractal_log


def signup_mail(username, promo_code):
    try:
        message = Mail(
            from_email="phil@tryfractal.com",
            to_emails=username,
            subject="Welcome to Fractal",
            html_content=render_template("on_signup.html", code=promo_code),
        )
        sendgrid_client = SendGridAPIClient(current_app.config["SENDGRID_API_KEY"])

        sendgrid_client.send(message)
    except Exception as e:
        fractal_log(
            function="signup_mail",
            label="ERROR",
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )
