from flask import Blueprint

from app import fractal_pre_process
from app.helpers.blueprint_helpers.mail.mail_post import (
    cancel_helper,
    computer_ready_helper,
    feedback_helper,
    forgot_password_helper,
    join_waitlist_helper,
    referral_mail_helper,
    trial_start_helper,
    verification_helper,
    waitlist_referral_helper,
)

mail_bp = Blueprint("mail_bp", __name__)


@mail_bp.route("/mail/<action>", methods=["POST"])
@fractal_pre_process
def mail(action, **kwargs):
    body = kwargs["body"]
    if action == "forgot":
        return forgot_password_helper(body["username"])
    elif action == "cancel":
        return cancel_helper(body["username"], body["feedback"])

    elif action == "verification":
        return verification_helper(body["username"], body["token"])

    elif action == "referral":
        return referral_mail_helper(body["username"], body["recipients"], body["code"])

    elif action == "feedback":
        return feedback_helper(body["username"], body["feedback"], body["type"])

    elif action == "trialStart":
        return trial_start_helper(body["username"], body["location"], body["code"])

    elif action == "computerReady":
        return computer_ready_helper(body["username"], body["date"], body["code"], body["location"])

    elif action == "joinWaitlist":
        return join_waitlist_helper(body["email"], body["name"], body["date"])

    elif action == "waitlistReferral":
        return waitlist_referral_helper(
            body["email"], body["name"], body["code"], body["recipient"]
        )
