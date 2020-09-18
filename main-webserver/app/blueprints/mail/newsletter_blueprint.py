from app import *
from app.helpers.blueprint_helpers.mail.mail_post import *
from app.helpers.blueprint_helpers.mail.mail_get import *

newsletter_bp = Blueprint("newsletter_bp", __name__)


@newsletter_bp.route("/newsletter/<action>", methods=["POST"])
@fractalPreProcess
def newsletter(action, **kwargs):
    body = kwargs["body"]
    return jsonify({}), SUCCESS
