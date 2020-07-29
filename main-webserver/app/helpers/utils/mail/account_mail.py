from app import *
from app.helpers.utils.general.logs import *


def signupMail(username, promo_code):
    try:
        internal_message = SendGridMail(
            from_email="phil@fractalcomputers.com",
            to_emails=username,
            subject="Welcome to Fractal",
            html_content=render_template("on_signup.html", code=promo_code),
        )
        sg = SendGridAPIClient(os.getenv("SENDGRID_API_KEY"))
        response = sg.send(internal_message)
    except Exception as e:
        fractalLog(
            function="signupMail",
            label="ERROR",
            logs="Mail send failed: Error code " + e.message,
            level=logging.ERROR,
        )

