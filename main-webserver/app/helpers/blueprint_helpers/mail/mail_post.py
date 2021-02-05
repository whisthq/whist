import logging

from datetime import timedelta
from sendgrid import SendGridAPIClient
from sendgrid.helpers.mail import Mail
from flask import current_app, jsonify, render_template
from flask_jwt_extended import create_access_token

from app.constants.http_codes import NOT_FOUND, SUCCESS, UNAUTHORIZED
from app.helpers.utils.general.logs import fractal_log
from app.models import User
from app.helpers.utils.mail.mail_client import MailClient, TemplateNotFound

def mail_helper(email_id, from_email, to_emails, email_args):
    mail_client = MailClient(current_app.config["SENDGRID_API_SECRET"])
    templates = mail_client.get_available_templates()

    if not email_id in templates.keys():
        raise TemplateNotFound

    mail_client.send_email(from_email, to_emails, templates[email_id].title, templates[email_id].url, email_args)
    











def forgot_password_helper(username, email_token=None):
    """
    Sends verification mail for resetting a password

    Parameters:
        username (str): the username
        email_token (optional, str): email verification token of user with username
                            'username', used for integration testing wtih frontend as
                            another check alongside their email containing @fractal.co

    Returns:
        json: request
            if fail or user not found, returns false
            if email_token exists and username is a developer, returns url and reset token
            otherwise returns true
    """
    user = User.query.get(username)

    if user:
        token = create_access_token(identity=username, expires_delta=timedelta(minutes=10))

        try:
            message = Mail(
                from_email=current_app.config["SENDGRID_DEFAULT_FROM"],
                to_emails=username,
                subject="Reset Your Password",
                html_content=render_template(
                    "on_password_forget.html", url=current_app.config["FRONTEND_URL"], token=token
                ),
            )
            sendgrid_client = SendGridAPIClient(current_app.config["SENDGRID_API_KEY"])

            sendgrid_client.send(message)
        except Exception as e:
            fractal_log(
                function="forgot_password_helper",
                label=username,
                logs="Mail send failed: Error code " + str(e),
                level=logging.ERROR,
            )
            return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

        if email_token:  # for frontend testing
            if email_token == user.token and "@fractal.co" in username:
                return (
                    jsonify(
                        {
                            "verified": True,
                            "token": token,
                            "url": current_app.config["FRONTEND_URL"],
                        }
                    ),
                    SUCCESS,
                )
        return jsonify({"verified": True}), SUCCESS
    else:
        return jsonify({"verified": False}), NOT_FOUND


def cancel_helper(user, feedback):
    title = "[CANCELLED PLAN + FEEDBACK] " + user + " has Just Cancelled Their Plan"
    if not feedback:
        feedback = "User did not submit feedback."

    try:
        message = Mail(
            from_email=current_app.config["SENDGRID_DEFAULT_FROM"],
            to_emails="support@fractal.co",
            subject=title,
            html_content=feedback,
        )
        sendgrid_client = SendGridAPIClient(current_app.config["SENDGRID_API_KEY"])

        sendgrid_client.send(message)
    except Exception as e:
        fractal_log(
            function="cancel_helper",
            label=user,
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def verification_helper(user, token):
    title = "[Fractal] Please Verify Your Email"
    url = current_app.config["FRONTEND_URL"] + "/verify?" + token
    # url = "https://localhost:3000/verify?" + token

    try:
        message = Mail(
            from_email=current_app.config["SENDGRID_DEFAULT_FROM"],
            to_emails=user,
            subject=title,
            html_content=render_template("on_email_verification.html", url=url),
        )
        sendgrid_client = SendGridAPIClient(current_app.config["SENDGRID_API_KEY"])

        sendgrid_client.send(message)
    except Exception as e:
        fractal_log(
            function="verification_helper",
            label=user,
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def referral_mail_helper(user, recipients, code):
    title = "Check out Fractal"

    try:
        message = Mail(
            from_email=user,
            to_emails=[{"email": email} for email in recipients],
            subject=title,
            html_content=render_template("on_referral.html", code=code, user=user),
        )
        sendgrid_client = SendGridAPIClient(current_app.config["SENDGRID_API_KEY"])

        sendgrid_client.send(message)
    except Exception as e:
        fractal_log(
            function="referral_mail_helper",
            label=user,
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def feedback_helper(user, feedback, feedback_type):
    title = "[{}] Feedback from {}".format(feedback_type, user)

    try:
        message = Mail(
            from_email=current_app.config["SENDGRID_DEFAULT_FROM"],
            to_emails="support@fractal.co",
            subject=title,
            html_content="<div>" + feedback + "</div>",
        )
        sendgrid_client = SendGridAPIClient(current_app.config["SENDGRID_API_KEY"])

        sendgrid_client.send(message)
    except Exception as e:
        fractal_log(
            function="feedback_helper",
            label=user,
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def trial_start_helper(user, location, code):
    try:
        message = Mail(
            from_email=current_app.config["SENDGRID_DEFAULT_FROM"],
            to_emails=user,
            subject="Your Free Trial has Started",
            html_content=render_template("on_purchase.html", location=location, code=code),
        )
        sendgrid_client = SendGridAPIClient(current_app.config["SENDGRID_API_KEY"])

        sendgrid_client.send(message)
    except Exception as e:
        fractal_log(
            function="trial_start_helper",
            label=user,
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    try:
        message = Mail(
            from_email=current_app.config["SENDGRID_DEFAULT_FROM"],
            to_emails="support@fractal.co",
            subject="[FREE TRIAL START] A new user, "
            + user
            + ", just signed up for the free trial.",
            html_content="<div>No action needed from our part at this point.</div>",
        )
        sendgrid_client = SendGridAPIClient(current_app.config["SENDGRID_API_KEY"])

        sendgrid_client.send(message)
    except Exception as e:
        fractal_log(
            function="trial_start_helper",
            label=user,
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def computer_ready_helper(user, date, code, location):
    title = "Your Fractal Subscription Is Ready!"

    try:
        message = Mail(
            from_email=current_app.config["SENDGRID_DEFAULT_FROM"],
            to_emails=user,
            subject=title,
            html_content=render_template(
                "on_cloud_pc_ready.html", date=date, code=code, location=location
            ),
        )
        sendgrid_client = SendGridAPIClient(current_app.config["SENDGRID_API_KEY"])

        sendgrid_client.send(message)
    except Exception as e:
        fractal_log(
            function="computer_ready_helper",
            label=user,
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    try:
        message = Mail(
            from_email=current_app.config["SENDGRID_DEFAULT_FROM"],
            to_emails="support@fractal.co",
            subject="" + user + " has signed up for a Fractal paid plan.",
            html_content="<div>{} has signed up for a Fractal paid plan.</div>".format(user),
        )
        sendgrid_client = SendGridAPIClient(current_app.config["SENDGRID_API_KEY"])

        sendgrid_client.send(message)
    except Exception as e:
        fractal_log(
            function="computer_ready_helper",
            label=user,
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def join_waitlist_helper(email, name, date):
    title = "Congrats! You're on the waitlist."

    try:
        message = Mail(
            from_email=current_app.config["SENDGRID_DEFAULT_FROM"],
            to_emails=email,
            subject=title,
            html_content=render_template("join_waitlist.html", name=name, date=date),
        )
        sendgrid_client = SendGridAPIClient(current_app.config["SENDGRID_API_KEY"])

        sendgrid_client.send(message)
    except Exception as e:
        fractal_log(
            function="join_waitlist_helper",
            label=email,
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def waitlist_referral_helper(email, name, code, recipient):
    title = name + " has invited you to join Fractal's waitlist!"

    try:
        message = Mail(
            from_email=current_app.config["SENDGRID_DEFAULT_FROM"],
            to_emails=recipient,
            subject=title,
            html_content=render_template("on_waitlist_referral.html", email=email, code=code),
        )
        sendgrid_client = SendGridAPIClient(current_app.config["SENDGRID_API_KEY"])

        sendgrid_client.send(message)
    except Exception as e:
        fractal_log(
            function="waitlist_referral_helper",
            label=email,
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS
