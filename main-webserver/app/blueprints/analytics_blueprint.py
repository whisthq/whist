from app import *
from app.helpers.blueprint_helpers.analytics_post import *

analytics_bp = Blueprint("analytics_bp", __name__)


@analytics_bp.route("/analytics/<action>", methods=["POST"])
@fractalPreProcess
def analytics_post(action, **kwargs):
    if action == "logs":
        body = json.loads(request.data)

        return logsHelper(body["filename"], body["sender"])
