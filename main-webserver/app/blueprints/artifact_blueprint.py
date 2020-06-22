from app import *
from app.helpers.blueprint_helpers.artifact_post import *
from app.celery.azure_resource_creation import *
from app.celery.azure_resource_deletion import *
from app.celery.azure_resource_state import *

artifact_bp = Blueprint("artifact_bp", __name__)


@artifact_bp.route("/artifact/<action>", methods=["POST"])
@fractalPreProcess
def artifact_post(action, **kwargs):
    if action == "deploy":
        return "deployed"
