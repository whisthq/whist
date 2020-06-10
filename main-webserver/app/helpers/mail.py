from app import *
from app.logger import *


def chargeFailedMail(username, custId, ID=-1):
    sendInfo(ID, "Sending charge failed email to {}".format(username))
    try:
        message = SendGridMail(
            from_email="noreply@fractalcomputers.com",
            to_emails=[username],
            subject="Your Payment is Overdue",
            html_content=render_template("charge_failed.html"),
        )
        sg = SendGridAPIClient(os.getenv("SENDGRID_API_KEY"))
        response = sg.send(message)
    except Exception as e:
        sendError(ID, "Mail send failed: Error code " + e.message)

    message = SendGridMail(
        from_email="noreply@fractalcomputers.com",
        to_emails=["support@fractalcomputers.com"],
        subject="Payment is overdue for " + username,
        html_content="<div>The charge has failed for account " + custId + "</div>",
    )
    try:
        sg = SendGridAPIClient(os.getenv("SENDGRID_API_KEY"))
        response = sg.send(message)
        sendInfo(ID, "Sent charge failed email to support")
    except Exception as e:
        sendError(ID, "Mail send failed: Error code " + e.message)


def trialEndingMail(user, ID=-1):
    sendInfo(ID, "Sending trial ending email to {}".format(user))
    message = SendGridMail(
        from_email="noreply@fractalcomputers.com",
        to_emails=user,
        subject="Your Free Trial is Ending",
        html_content=render_template("trial_ending.html"),
    )
    try:
        sg = SendGridAPIClient(os.getenv("SENDGRID_API_KEY"))
        response = sg.send(message)
        sendInfo(ID, "Sent trial ending email to customer")
    except Exception as e:
        sendError(ID, "Mail send failed: Error code " + e.message)


def trialEndedMail(username, ID=-1):
    message = SendGridMail(
        from_email="noreply@fractalcomputers.com",
        to_emails=username,
        subject="Your Free Trial has Ended",
        html_content=render_template("trial_ended.html"),
    )
    try:
        sg = SendGridAPIClient(os.getenv("SENDGRID_API_KEY"))
        response = sg.send(message)
        sendInfo(ID, "Sent trial ended email to {}".format(username))
    except Exception as e:
        sendError(
            ID, "Mail send failed: Error code " + e.message,
        )


def creditAppliedMail(username, ID=-1):
    internal_message = SendGridMail(
        from_email="support@fractalcomputers.com",
        to_emails=username,
        subject="Someone Applied Your Promo Code — Here's $35.00!",
        html_content=render_template("on_credit_applied.html"),
    )

    try:
        sg = SendGridAPIClient(os.getenv("SENDGRID_API_KEY"))
        response = sg.send(internal_message)
        sendInfo(ID, "Sent credit applied email to {}".format(username))
    except Exception as e:
        sendError(
            ID, "Mail send failed: Error code " + e.message,
        )

    return jsonify({"status": 200}), 200
