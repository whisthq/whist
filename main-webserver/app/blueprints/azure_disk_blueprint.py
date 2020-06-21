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
    if action == "clone":
        # Clone a Fractal disk

        resource_group = os.getenv("VM_GROUP")
        if "resource_group" in body.keys():
            resource_group = kwargs["body"]["resource_group"]

        task = cloneDisk.apply_async(
            [
                kwargs["body"]["username"],
                kwargs["body"]["location"],
                kwargs["body"]["vm_size"],
                kwargs["body"]["operating_system"],
                kwargs["body"]["apps"],
                resource_group,
            ]
        )

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED

    elif action == "swap":
        # Swap a disk onto a specified VM

        disk_name, vm_name, resource_group = (
            kwargs["body"]["disk_name"],
            kwargs["body"]["vm_name"],
            kwargs["body"]["resource_group"],
        )

        task = swapSpecificDisk.apply_async([vm_name, disk_name, resource_group])

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED
