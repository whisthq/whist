import os

from flask import Blueprint
from flask.json import jsonify
from flask_jwt_extended import get_jwt_identity, jwt_required

from app import fractalPreProcess
from app.celery.azure_resource_creation import createVM
from app.celery.azure_resource_deletion import deleteVM
from app.celery.azure_resource_modification import runPowershell
from app.celery.azure_resource_state import deallocateVM, pingHelper, restartVM
from app.celery.azure_resource_state import startVM
from app.constants.config import VM_GROUP
from app.constants.http_codes import ACCEPTED, BAD_REQUEST
from app.helpers.blueprint_helpers.azure.azure_vm_get import ipHelper
from app.helpers.blueprint_helpers.azure.azure_vm_get import protocolInfoHelper
from app.helpers.blueprint_helpers.azure.azure_vm_post import devHelper
from app.helpers.utils.general.auth import adminRequired, fractalAuth

azure_vm_bp = Blueprint("azure_vm_bp", __name__)


@azure_vm_bp.route("/vm/ping", methods=["POST"])
@fractalPreProcess
def vm_ping(**kwargs):
    # Receives pings from active VMs

    available, vm_ip = kwargs["body"]["available"], kwargs["received_from"]

    if "resource_group" in kwargs["body"].keys():
        resource_group = kwargs["body"]["resource_group"]

    version = None
    if "version" in kwargs["body"].keys():
        version = kwargs["body"]["version"]

    task = pingHelper.delay(available, vm_ip, version)

    return jsonify({"ID": task.id}), ACCEPTED


@azure_vm_bp.route("/vm/restart", methods=["POST"])
@fractalPreProcess
@jwt_required
@fractalAuth
def vm_restart(**kwargs):
    # Restarts an Azure VM

    vm_name = kwargs["body"]["vm_name"]
    resource_group = VM_GROUP
    if "resource_group" in kwargs["body"].keys():
        resource_group = kwargs["body"]["resource_group"]

    task = restartVM.apply_async([vm_name, resource_group])

    if not task:
        return jsonify({"ID": None}), BAD_REQUEST

    return jsonify({"ID": task.id}), ACCEPTED


@azure_vm_bp.route("/vm/<action>", methods=["POST"])
@fractalPreProcess
@jwt_required
@adminRequired
def vm_post(action, **kwargs):
    if action == "create":
        # Creates an Azure VM

        vm_size = kwargs["body"]["vm_size"]
        location = kwargs["body"]["location"]
        resource_group = kwargs["body"]["resource_group"]

        operating_system = (
            "Windows"
            if kwargs["body"]["operating_system"].upper() == "WINDOWS"
            else "Linux"
        )

        admin_password = None
        if "admin_password" in kwargs["body"].keys():
            admin_password = kwargs["body"]["admin_password"]

        task = createVM.apply_async(
            [vm_size, location, operating_system, admin_password, resource_group]
        )

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED
    elif action == "delete":
        # Deletes an Azure VM

        vm_name, delete_disk, resource_group = (
            kwargs["body"]["vm_name"],
            kwargs["body"]["delete_disk"],
            kwargs["body"]["resource_group"],
        )

        task = deleteVM.apply_async([vm_name, delete_disk, resource_group])

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED
    elif action == "start":
        # Starts an Azure VM

        vm_name = kwargs["body"]["vm_name"]
        resource_group = VM_GROUP
        if "resource_group" in kwargs["body"].keys():
            resource_group = kwargs["body"]["resource_group"]

        task = startVM.apply_async([vm_name, resource_group])

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED
    elif action == "restart":
        # Restarts an Azure VM

        vm_name = kwargs["body"]["vm_name"]
        resource_group = os.getenv("VM_GROUP")
        if "resource_group" in kwargs["body"].keys():
            resource_group = kwargs["body"]["resource_group"]

        task = restartVM.apply_async([vm_name, resource_group])

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED
    elif action == "deallocate":
        # Deallocates an Azure VM

        vm_name = kwargs["body"]["vm_name"]
        resource_group = VM_GROUP
        if "resource_group" in kwargs["body"].keys():
            resource_group = kwargs["body"]["resource_group"]

        task = deallocateVM.apply_async([vm_name, resource_group])

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED

    elif action == "dev":
        # Toggles dev mode for a specified VM

        vm_name, dev = kwargs["body"]["vm_name"], kwargs["body"]["dev"]

        output = devHelper(vm_name, dev)

        return jsonify(output), output["status"]
    elif action == "command":
        # Runs a powershell script on a VM

        vm_name, powershell_script, resource_group = (
            kwargs["body"]["vm_name"],
            kwargs["body"]["command"],
            kwargs["body"]["resource_group"],
        )

        task = runPowershell.apply_async([vm_name, powershell_script, resource_group])

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED


@azure_vm_bp.route("/vm/<action>", methods=["GET"])
@fractalPreProcess
def vm_get(action, **kwargs):
    if action == "ip":
        # Gets the IP address of a VM using Azure SDK

        vm_name = request.args.get("vm_name")

        current_user = get_jwt_identity()

        resource_group = request.args.get("resource_group")

        output = ipHelper(vm_name, resource_group)

        return jsonify(output), output["status"]

    elif action == "protocol_info":
        output = protocolInfoHelper(kwargs["received_from"])

        return jsonify(output), output["status"]
