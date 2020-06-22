from app import *
from app.helpers.blueprint_helpers.artifact_post import *
from app.celery.azure_resource_creation import *
from app.celery.azure_resource_modification import *
from app.celery.azure_resource_state import *

artifact_bp = Blueprint("artifact_bp", __name__)


@artifact_bp.route("/artifact/<action>", methods=["POST"])
@fractalPreProcess
def artifact_post(action, **kwargs):
    if action == "deploy":
        artifact_name, run_id, vm_name = (
            kwargs["body"]["artifact_name"],
            kwargs["body"]["run_id"],
            kwargs["body"]["vm_name"],
        )

    resource_group = "FractalProtocolCI"
    if "resource_group" in kwargs["body"].keys():
        resource_group = kwargs["body"]["resource_group"]

    task = deployArtifact.apply_async([vm_name, artifact_name, run_id, resource_group])

    if not task:
        return jsonify({"ID": None}), BAD_REQUEST

    return jsonify({"ID": task.id}), ACCEPTED
