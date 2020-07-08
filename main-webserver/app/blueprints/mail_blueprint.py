from app import *
from app.helpers.blueprint_helpers.mail_post import *

mail_bp = Blueprint("mail_bp", __name__)


@mail_bp.route("/mail/<action>", methods=["POST"])
@fractalPreProcess
def mail(action, **kwargs):
    body = kwargs["body"]
    if action == "forgot":
        return forgotPasswordHelper(body["username"])

    elif action == "reset":
        resetPasswordHelper(body["username"], body["password"])
        return jsonify({"status": 200}), 200
        
    elif action == "cancel":
        return cancelHelper(body["username"], body["feedback"])

    elif action == "verification":
        return verificationHelper(body["username"], body["token"])
        
    elif action == "referral":
        return referralHelper(body["username"], body["recipients"], body["code"])

    elif action == "feedback":
        return feedbackHelper(body["username"], body["feedback"], body["type"])

    elif action == "trialStart":
        return trialStartHelper(body["username"], body["location"], body["code"])
    
    elif action == "signup":
        return signupMailHelper(body["username"], body["code"])

    elif action == "computerReady":
        return computerReadyHelper(body["username"], body["date"], body["code"], body["location"])


@mail_bp.route("/newsletter/<action>", methods=["POST"])
@fractalPreProcess
def newsletter(action, **kwargs):
    body = kwargs["body"]
    if action == "subscribe":
        newsletterSubscribe(body["username"])
        return jsonify({"status": 200}), 200

    elif action == "unsubscribe":
        newsletterUnsubscribe(body["username"])
        return jsonify({"status": 200}), 200