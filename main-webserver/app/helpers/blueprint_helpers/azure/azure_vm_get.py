from app import *
from app.helpers.utils.azure.azure_general import *

from app.models.hardware import *


def ipHelper(vm_name, resource_group):
    """Fetches the IP address of a VM

    Args:
        vm_name (str): Name of VM

    Returns:
        json: IP address
    """

    vm_ip = getVMIP(vm_name, resource_group)
    return {"public_ip": vm_ip, "status": SUCCESS}


def protocolInfoHelper(ip_address):
    vm = UserVM.query.filter_by(ip=ip_address).first()

    if not vm:
        return {
            "allow_autoupdate": True,
            "has_dedicated_vm": False,
            "branch": "dev",
            "status": NOT_FOUND,
            "using_stun": False,
            "private_key": None,
            "access_token": None,
            "refresh_token": None,
        }
    else:
        disk = OSDisk.query.get(vm.disk_id)

        access_token, refresh_token = getAccessTokens(DASHBOARD_USERNAME + "@gmail.com")

        if disk:
            return {
                "allow_autoupdate": disk.allow_autoupdate,
                "has_dedicated_vm": disk.has_dedicated_vm,
                "branch": disk.branch,
                "status": SUCCESS,
                "using_stun": disk.using_stun,
                "private_key": disk.rsa_private_key,
                "access_token": access_token,
                "refresh_token": refresh_token,
            }
        else:
            fractalLog(
                function="protocolInfoHelper",
                label=vm["username"],
                logs="Error fetching disk settings for disk {disk_name}: {error}".format(
                    disk_name=str(vm["disk_name"]), error="error"
                ),
                level=logging.ERROR,
            )

            return {
                "allow_autoupdate": True,
                "has_dedicated_vm": False,
                "status": NOT_FOUND,
                "using_stun": False,
                "private_key": None,
                "access_token": None,
                "refresh_token": None,
            }
