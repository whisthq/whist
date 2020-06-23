from app import *
from app.helpers.utils.azure.azure_general import *


def ipHelper(vm_name, resource_group):
    """Fetches the IP address of a VM

    Args:
        vm_name (str): Name of VM

    Returns:
        json: IP address
    """

    vm_ip = getVMIP(vm_name, resource_group)
    return {"public_ip": vm_ip, "status": SUCCESS}
