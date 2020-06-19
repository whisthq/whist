from app import *
from app.helpers.blueprint_helpers.azure_vm_get import *
from app.helpers.blueprint_helpers.azure_vm_post import *
from app.celery.azure_resource_creation import *
from app.celery.azure_resource_deletion import *
from app.celery.azure_resource_modification import *

azure_disk_bp = Blueprint("azure_disk_bp", __name__)


@azure_disk_bp.route("/azure_disk/<action>", methods=["POST"])
@fractalPreProcess
def azure_disk_post(action, **kwargs):
    if action == "swap":
        disk_name, vm_name = kwargs["body"]["disk_name"], kwargs["body"]["vm_name"]

        task = swapSpecificDisk.apply_async([vm_name, disk_name])

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED
