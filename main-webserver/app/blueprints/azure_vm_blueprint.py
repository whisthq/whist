from app import *
from app.helpers.blueprint_helpers.azure_vm_get import *
from app.helpers.blueprint_helpers.azure_vm_post import *
from app.celery.azure_resource_creation import *
from app.celery.azure_resource_deletion import *

azure_vm_bp = Blueprint("azure_vm_bp", __name__)


@azure_vm_bp.route("/azure_vm/<action>", methods=["POST"])
@fractalPreProcess
def azure_vm_post(action, **kwargs):
    if action == "create":
        # Creates an Azure VM

        vm_size = kwargs["body"]["vm_size"]
        location = kwargs["body"]["location"]
        operating_system = (
            "Windows"
            if kwargs["body"]["operating_system"].upper() == "WINDOWS"
            else "Linux"
        )

        admin_password = None
        if "admin_password" in kwargs["body"].keys():
            admin_password = kwargs["body"]["admin_password"]

        task = createVM.apply_async(
            [vm_size, location, operating_system, admin_password]
        )

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED
    elif action == "delete":
        # Deletes an Azure VM

        vm_name, delete_disk = kwargs["body"]["vm_name"], kwargs["body"]["delete_disk"]

        task = deleteVM.apply_async([vm_name, delete_disk])

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED
    elif action == "dev":
        # Toggles dev mode for a specified VM

        vm_name, dev = kwargs["body"]["vm_name"], kwargs["body"]["dev"]

        output = devHelper(vm_name, dev)

        return jsonify(output), output["status"]
    elif action == "connectionStatus":
        # Receives pings from active VMs

        available, vm_ip = kwargs["body"]["available"], kwargs["received_from"]
        version = None
        if "version" in kwargs["body"].keys():
            version = kwargs["body"]["version"]

        output = connectionStatusHelper(available, vm_ip, version)

        return jsonify(output), output["status"]


@azure_vm_bp.route("/azure_vm/<action>", methods=["GET"])
@fractalPreProcess
def azure_vm_get(action, **kwargs):
    if action == "ip":
        # Gets the IP address of a VM using Azure SDK

        vm_name = request.args.get("vm_name")

        output = ipHelper(vm_name)

        return jsonify(output), output["status"]
