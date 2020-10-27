import logging
import string

from datetime import datetime as dt
from datetime import timedelta, timezone

from flask import current_app, jsonify, render_template
from jose import jwt

from app import mail
from app.constants.config import FRONTEND_URL
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
                "sub": username,
                "exp": (dt.now() + timedelta(minutes=10)).replace(tzinfo=timezone.utc).timestamp(),
            },
            current_app.config["JWT_SECRET_KEY"],
        )
        timeIssued = dt.now().strftime("%m-%d-%Y, %H:%M:%S")

        try:
            mail.send_email(
                to_email=username,
                subject="Reset Your Password",
                html=render_template("on_password_forget.html", url=FRONTEND_URL, token=token),
            )
        except Exception as e:
            fractalLog(
                function="forgotPasswordHelper",
                label=username,
                logs="Mail send failed: Error code " + e.message,
                level=logging.ERROR,
            )
            return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

        return jsonify({"verified": True}), SUCCESS
    else:
        return jsonify({"verified": False}), NOT_FOUND


def cancelHelper(user, feedback):
    title = "[CANCELLED PLAN + FEEDBACK] " + user + " has Just Cancelled Their Plan"

    try:
        mail.send_email(
            to_email="support@tryfractal.com",
            subject=title,
            html=feedback,
        )
    except Exception as e:
        fractalLog(
            function="cancelHelper",
            label=user,
            logs="Mail send failed: Error code " + e.message,
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def verificationHelper(user, token):
    title = "[Fractal] Please Verify Your Email"
    url = FRONTEND_URL + "/verify?" + token
    # url = "https://localhost:3000/verify?" + token

    try:
        mail.send_email(
            to_email=user,
            subject=title,
            html=render_template("on_email_verification.html", url=url),
        )
    except Exception as e:
        fractalLog(
            function="verificationHelper",
            label=user,
            logs="Mail send failed: Error code " + e.message,
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def referralMailHelper(user, recipients, code):
    title = "Check out Fractal"

    try:
        mail.send_email(
            from_email=user,
            to_email=[{"email": email} for email in recipients],
            subject=title,
            html=render_template("on_referral.html", code=code, user=user),
        )
    except Exception as e:
        fractalLog(
            function="referralMailHelper",
            label=user,
            logs="Mail send failed: Error code " + e.message,
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def feedbackHelper(user, feedback, feedback_type):
    title = "[{}] Feedback from {}".format(feedback_type, user)

    try:
        mail.send_email(
            to_email="support@tryfractal.com",
            subject=title,
            html="<div>" + feedback + "</div>",
        )
    except Exception as e:
        fractalLog(
            function="feedbackHelper",
            label=user,
            logs="Mail send failed: Error code " + e.message,
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def trialStartHelper(user, location, code):
    try:
        mail.send_email(
            to_email=user,
            subject="Your Free Trial has Started",
            html=render_template("on_purchase.html", location=location, code=code),
        )
    except Exception as e:
        fractalLog(
            function="trialStartHelper",
            label=user,
            logs="Mail send failed: Error code " + e.message,
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    try:
        mail.send_email(
            to_email="support@tryfractal.com",
            subject="[FREE TRIAL START] A new user, "
            + user
            + ", just signed up for the free trial.",
            html="<div>No action needed from our part at this point.</div>",
        )
    except Exception as e:
        fractalLog(
            function="trialStartHelper",
            label=user,
            logs="Mail send failed: Error code " + e.message,
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def computerReadyHelper(user, date, code, location):
    title = "Your Fractal Subscription Is Ready!"

    try:
        mail.send_email(
            from_email="support@tryfractalcom",
            to_email=user,
            subject=title,
            html=render_template("on_cloud_pc_ready.html", date=date, code=code, location=location),
        )
    except Exception as e:
        fractalLog(
            function="computerReadyHelper",
            label=user,
            logs="Mail send failed: Error code " + e.message,
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    try:
        mail.send_email(
            to_email="support@tryfractal.com",
            subject="" + user + " has signed up for a Fractal paid plan.",
            html="<div>{} has signed up for a Fractal paid plan.</div>".format(user),
        )
    except Exception as e:
        fractalLog(
            function="computerReadyHelper",
            label=user,
            logs="Mail send failed: Error code " + e.message,
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def joinWaitlistHelper(email, name, date):
    title = "Congrats! You're on the waitlist."

    try:
        mail.send_email(
            from_email="support@tryfractal.com",
            to_email=email,
            subject=title,
            html=render_template("join_waitlist.html", name=name, date=date),
        )
    except Exception as e:
        fractalLog(
            function="joinWaitlistHelper",
            label=email,
            logs="Mail send failed: Error code " + e.message,
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def waitlistReferralHelper(email, name, code, recipient):
    title = name + " has invited you to join Fractal's waitlist!"

    try:
        mail.send_email(
            from_email=email,
            to_email=recipient,
            subject=title,
            html=render_template("on_waitlist_referral.html", email=email, code=code),
        )
    except Exception as e:
        fractalLog(
            function="waitlistReferralHelper",
            label=email,
            logs="Mail send failed: Error code " + e.message,
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS
