import logging

from flask import render_template

from app import mail
from app.helpers.utils.general.logs import fractalLog


def chargeFailedMail(username, custId):
    fractalLog(
        function="chargeFailedMail",
        label=username,
        logs="Sending charge failed email to {}".format(username),
    )

    try:
        mail.send_email(
            to_email=username,
            subject="Your Payment is Overdue",
            html=render_template("charge_failed.html"),
        )
    except Exception as e:
        fractalLog(
            function="chargeFailedMail",
            label=username,
            logs="Mail send failed: Error code " + str(e),
            level=logging.ERROR,
        )

    try:
        mail.send_email(
            to_email="support@tryfractal.com",
            subject="Payment is overdue for " + username,
            html="<div>The charge has failed for account " + custId + "</div>",
        )
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
        mail.send_email(
            to_email="support@tryfractal.com",
            subject="Payment recieved from " + username,
            html="<div>The charge has succeeded for account " + custId + "</div>",
        )
        fractalLog(
            function="chargeSuccessMail",
            label="Stripe",
            logs="Sent charge success email to support",
        )
    except Exception as e:
<<<<<<< HEAD
        fractalLog(
            function="chargeSuccessMail", label="Stripe", logs=str(e), level=logging.ERROR
        )
=======
        fractalLog(function="chargeSuccessMail", label="Stripe", logs=str(e), level=logging.ERROR)
>>>>>>> 0cad5df4234663e13d7f5aca7aa2f088bca75f88


def trialEndingMail(user):
    fractalLog(
        function="trialEndingMail",
        label="Stripe",
        logs="Sending trial ending email to {}".format(user),
    )

    try:
        mail.send_email(
            to_email=user,
            subject="Your Free Trial is Ending",
            html=render_template("trial_ending.html"),
        )
        fractalLog(
            function="trialEndingMail", label="Stripe", logs="Sent trial ending email to customer"
        )
    except Exception as e:
        fractalLog(function="trialEndingMail", label="Stripe", logs=str(e), level=logging.ERROR)


def trialEndedMail(username):
    try:
        mail.send_email(
            to_email=username,
            subject="Your Free Trial has Ended",
            html=render_template("trial_ended.html"),
        )
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
        mail.send_email(
            from_email="support@tryfractal.com",
            to_email=username,
            subject="Someone Applied Your Promo Code — Here's $35.00!",
            html=render_template("on_credit_applied.html"),
        )
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
        mail.send_email(
            to_email=username,
            subject="Your plan change was successful",
            html=render_template("plan_changed.html", plan=newPlan),
        )
        fractalLog(
            function="planChangeMail",
            label="Mail",
            logs="Sent plan changed email to {}".format(username),
        )
    except Exception as e:
        fractalLog(function="planChangeMail", label="Mail", logs=str(e), level=logging.ERROR)
