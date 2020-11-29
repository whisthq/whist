import logging

from flask import render_template
from sendgrid import SendGridAPIClient
from sendgrid.helpers.mail import Mail

from app.constants.config import SENDGRID_API_KEY, SENDGRID_EMAIL
from app.helpers.utils.general.logs import fractalLog


def chargeFailedMail(username, custId):
    fractalLog(
        function="chargeFailedMail",
        label=username,
        logs="Sending charge failed email to {}".format(username),
    )

    try:
        message = Mail(
            from_email=SENDGRID_EMAIL,
            to_emails=username,
            subject="Your Payment is Overdue",
            html_content=render_template("charge_failed.html"),
        )
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(message)
    except Exception as e:
        fractalLog(
            function="chargeFailedMail",
            label=username,
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )

    try:
        message = Mail(
            from_email=SENDGRID_EMAIL,
            to_emails="support@tryfractal.com",
            subject="Payment is overdue for " + username,
            html_content="<div>The charge has failed for account " + custId + "</div>",
        )
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(message)
        fractalLog(
            function="chargeFailedMail", label=username, logs="Sent charge failed email to support"
        )
    except Exception as e:
        fractalLog(
            function="chargeFailedMail",
            label=username,
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )


def chargeSuccessMail(username, custId):
    try:
        message = Mail(
            from_email=SENDGRID_EMAIL,
            to_emails="support@tryfractal.com",
            subject="Payment recieved from " + username,
            html_content="<div>The charge has succeeded for account " + custId + "</div>",
        )
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(message)
        fractalLog(
            function="chargeSuccessMail",
            label="Stripe",
            logs="Sent charge success email to support",
        )
    except Exception as e:
        fractalLog(function="chargeSuccessMail", label="Stripe", logs=str(e), level=logging.ERROR)


def trialEndingMail(user):
    fractalLog(
        function="trialEndingMail",
        label="Stripe",
        logs="Sending trial ending email to {}".format(user),
    )

    try:
        message = Mail(
            from_email=SENDGRID_EMAIL,
            to_emails=user,
            subject="Your Free Trial is Ending",
            html_content=render_template("trial_ending.html"),
        )
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(message)
        fractalLog(
            function="trialEndingMail", label="Stripe", logs="Sent trial ending email to customer"
        )
    except Exception as e:
        fractalLog(function="trialEndingMail", label="Stripe", logs=str(e), level=logging.ERROR)


def trialEndedMail(username):
    try:
        message = Mail(
            from_email=SENDGRID_EMAIL,
            to_emails=username,
            subject="Your Free Trial has Ended",
            html_content=render_template("trial_ended.html"),
        )
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(message)
        fractalLog(
            function="trialEndedMail",
            label="Stripe",
            logs="Sent trial ended email to {}".format(username),
        )
    except Exception as e:
        fractalLog(function="trialEndedMail", label="Stripe", logs=str(e), level=logging.ERROR)


def creditAppliedMail(username):
    """Sends an email that lets the customer know that credit has been applied

    Args:
        username (str): The email to send to

    Returns:
        int: 0 for success, 1 for failure
    """

    try:
        message = Mail(
            from_email=SENDGRID_EMAIL,
            to_emails=username,
            subject="Someone Applied Your Promo Code — Here's $35.00!",
            html_content=render_template("on_credit_applied.html"),
        )
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(message)
        fractalLog(
            function="creditAppliedMail",
            label="Mail",
            logs="Sent credit applied email to {}".format(username),
        )
    except Exception as e:
        fractalLog(function="creditAppliedMail", label="Mail", logs=str(e), level=logging.ERROR)
        return 1

    return 0


def planChangeMail(username, newPlan):
    try:
        message = Mail(
            from_email=SENDGRID_EMAIL,
            to_emails=username,
            subject="Your plan change was successful",
            html_content=render_template("plan_changed.html", plan=newPlan),
        )
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(message)
        fractalLog(
            function="planChangeMail",
            label="Mail",
            logs="Sent plan changed email to {}".format(username),
        )
    except Exception as e:
        fractalLog(function="planChangeMail", label="Mail", logs=str(e), level=logging.ERROR)
