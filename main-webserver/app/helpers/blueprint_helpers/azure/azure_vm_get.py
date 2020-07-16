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


def protocolInfoHelper(ip_address):

    output = fractalSQLSelect(table_name="v_ms", params={"ip": ip_address})

    if not output["success"] or not output["rows"]:
        output = fractalSQLSelect(
            table_name="VMs_FractalProtocolCI", params={"ip": ip_address}
        )

        if not output["success"] or not output["rows"]:
            output = fractalSQLSelect(table_name="v_ms", params={"ip": ip_address})

            return {
                "dev": True,
                "branch": "dev",
                "status": NOT_FOUND,
                "using_stun": False,
                "access_token": None,
                "refresh_token": None,
            }
    else:
        vm_info = output["rows"][0]

        output = fractalSQLSelect(
            table_name="disk_settings", params={"disk_name": vm_info["disk_name"]}
        )

        access_token, refresh_token = getAccessTokens(
            str(vm_info["username"])
        )

        if output["rows"] and output["success"]:
            return {
                "dev": vm_info["dev"],
                "branch": output["rows"][0]["branch"],
                "status": SUCCESS,
                "using_stun": output["rows"][0]["using_stun"],
                "access_token": access_token,
                "refresh_token": refresh_token,
            }
        else:
            fractalLog(
                function="protocolInfoHelper",
                label=vm_info["username"],
                logs="Error fetching disk settings for disk {disk_name}: {error}".format(
                    disk_name=disk_name, error=output["error"]
                ),
                level=logging.ERROR,
            )

            return {
                "dev": vm_info["dev"],
                "branch": "dev",
                "status": NOT_FOUND,
                "using_stun": False,
                "access_token": None,
                "refresh_token": None,
            }
