import logging

from flask import render_template

from app import mail
from app.helpers.utils.general.logs import fractalLog
from app.models import User


def signupMail(username, promo_code):
    try:
        mail.send_email(
            from_email="phil@tryfractal.com",
            to_email=username,
            subject="Welcome to Fractal",
            html=render_template("on_signup.html", code=promo_code),
        )
    except Exception as e:
        fractalLog(
            function="signupMail",
            label="ERROR",
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )
