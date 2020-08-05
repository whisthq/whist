from app import *
from app.helpers.blueprint_helpers.mail.mail_post import *
from app.helpers.blueprint_helpers.mail.mail_get import *

newsletter_bp = Blueprint("newsletter_bp", __name__)


@newsletter_bp.route("/newsletter/<action>", methods=["POST"])
@fractalPreProcess
def newsletter(action, **kwargs):
    body = kwargs["body"]
    if action == "subscribe":
        newsletterSubscribe(body["username"])
        return jsonify({"status": SUCCESS}), SUCCESS

    elif action == "unsubscribe":
        newsletterUnsubscribe(body["username"])
        return jsonify({"status": SUCCESS}), SUCCESS


@newsletter_bp.route("/newsletter/<action>", methods=["GET"])
def newsletterGet(action):
    if action == "user":
        username = request.args.get("username")
        return checkUserSubscribed(username)
