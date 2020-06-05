from app.tasks import *
from app import *
from app.helpers.versions import *
from app.logger import *

version_bp = Blueprint("version_bp", __name__)


@version_bp.route("/version", methods=["POST", "GET"])
@generateID
@logRequestInfo
def version(**kwargs):
    if request.method == "POST":
        body = request.get_json()
        setBranchVersion(body["branch"], body["version"], kwargs["ID"])
        return jsonify({"status": 200}), 200
    elif request.method == "GET":
        try:
            versions = getAllVersions(kwargs["ID"])
            sendInfo(kwargs["ID"], "Versions found")
            return ({"versions": versions}), 200
        except:
            sendError(kwargs["ID"], "Versions not found")
            return ({"versions": None}), 404
