import logging
import string

from datetime import datetime as dt
from datetime import timedelta, timezone
from sendgrid import SendGridAPIClient
from sendgrid.helpers.mail import Mail
from flask import current_app, jsonify, render_template
from jose import jwt

from app import mail
from app.constants.config import FRONTEND_URL, SENDGRID_API_KEY, SENDGRID_EMAIL
from app.constants.http_codes import NOT_FOUND, SUCCESS, UNAUTHORIZED
from app.helpers.utils.general.logs import fractalLog
from app.models import User


def forgotPasswordHelper(username):
    user = User.query.get(username)

    if user:
        upperCase = string.ascii_uppercase
        lowerCase = string.ascii_lowercase
        numbers = "1234567890"
        token = jwt.encode(
            {
                "email": username,
                "exp": (dt.now() + timedelta(minutes=10)).replace(tzinfo=timezone.utc).timestamp(),
            },
            current_app.config["JWT_SECRET_KEY"],
        )
        timeIssued = dt.now().strftime("%m-%d-%Y, %H:%M:%S")

        try:
            message = Mail(
                from_email=SENDGRID_EMAIL,
                to_emails=username,
                subject="Reset Your Password",
                html_content=render_template(
                    "on_password_forget.html", url=FRONTEND_URL, token=token
                ),
            )
            sg = SendGridAPIClient(SENDGRID_API_KEY)
            response = sg.send(message)
        except Exception as e:
            fractalLog(
                function="forgotPasswordHelper",
                label=username,
                logs="Mail send failed: Error code " + str(e),
                level=logging.ERROR,
            )
            return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

        return jsonify({"verified": True}), SUCCESS
    else:
        return jsonify({"verified": False}), NOT_FOUND


def cancelHelper(user, feedback):
    title = "[CANCELLED PLAN + FEEDBACK] " + user + " has Just Cancelled Their Plan"

    try:
        message = Mail(
            from_email=SENDGRID_EMAIL,
            to_emails="support@tryfractal.com",
            subject=title,
            html_content=feedback,
        )
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(message)
    except Exception as e:
        fractalLog(
            function="cancelHelper",
            label=user,
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def verificationHelper(user, token):
    title = "[Fractal] Please Verify Your Email"
    url = FRONTEND_URL + "/verify?" + token
    # url = "https://localhost:3000/verify?" + token
    
    try:
        message = Mail(
            from_email=SENDGRID_EMAIL,
            to_emails=user,
            subject=title,
            html_content=render_template("on_email_verification.html", url=url),
        )
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(message)
    except Exception as e:
        fractalLog(
            function="verificationHelper",
            label=user,
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def referralMailHelper(user, recipients, code):
    title = "Check out Fractal"

    try:
        message = Mail(
            from_email=user,
            to_emails=[{"email": email} for email in recipients],
            subject=title,
            html_content=render_template("on_referral.html", code=code, user=user),
        )
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(message)
    except Exception as e:
        fractalLog(
            function="referralMailHelper",
            label=user,
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def feedbackHelper(user, feedback, feedback_type):
    title = "[{}] Feedback from {}".format(feedback_type, user)

    try:
        message = Mail(
            from_email=SENDGRID_EMAIL,
            to_emails="support@tryfractal.com",
            subject=title,
            html_content="<div>" + feedback + "</div>",
        )
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(message)
    except Exception as e:
        fractalLog(
            function="feedbackHelper",
            label=user,
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def trialStartHelper(user, location, code):
    try:
        message = Mail(
            from_email=SENDGRID_EMAIL,
            to_emails=user,
            subject="Your Free Trial has Started",
            html_content=render_template("on_purchase.html", location=location, code=code),
        )
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(message)
    except Exception as e:
        fractalLog(
            function="trialStartHelper",
            label=user,
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    try:
        message = Mail(
            from_email=SENDGRID_EMAIL,
            to_emails="support@tryfractal.com",
            subject="[FREE TRIAL START] A new user, "
            + user
            + ", just signed up for the free trial.",
            html_content="<div>No action needed from our part at this point.</div>",
        )
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(message)
    except Exception as e:
        fractalLog(
            function="trialStartHelper",
            label=user,
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def computerReadyHelper(user, date, code, location):
    title = "Your Fractal Subscription Is Ready!"

    try:
        message = Mail(
            from_email=SENDGRID_EMAIL,
            to_emails=user,
            subject=title,
            html_content=render_template(
                "on_cloud_pc_ready.html", date=date, code=code, location=location
            ),
        )
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(message)
    except Exception as e:
        fractalLog(
            function="computerReadyHelper",
            label=user,
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    try:
        message = Mail(
            from_email=SENDGRID_EMAIL,
            to_emails="support@tryfractal.com",
            subject="" + user + " has signed up for a Fractal paid plan.",
            html_content="<div>{} has signed up for a Fractal paid plan.</div>".format(user),
        )
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(message)
    except Exception as e:
        fractalLog(
            function="computerReadyHelper",
            label=user,
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def joinWaitlistHelper(email, name, date):
    title = "Congrats! You're on the waitlist."

    try:
        message = Mail(
            from_email=SENDGRID_EMAIL,
            to_emails=email,
            subject=title,
            html_content=render_template("join_waitlist.html", name=name, date=date),
        )
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(message)
    except Exception as e:
        fractalLog(
            function="joinWaitlistHelper",
            label=email,
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def waitlistReferralHelper(email, name, code, recipient):
    title = name + " has invited you to join Fractal's waitlist!"

    try:
        message = Mail(
            from_email=SENDGRID_EMAIL,
            to_emails=recipient,
            subject=title,
            html_content=render_template("on_waitlist_referral.html", email=email, code=code),
        )
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(message)
    except Exception as e:
        fractalLog(
            function="waitlistReferralHelper",
            label=email,
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS
