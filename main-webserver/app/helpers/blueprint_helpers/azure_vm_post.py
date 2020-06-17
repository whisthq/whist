from app import *


def devHelper(vm_name, dev):
    """Toggles the dev mode of a VM

    Args:
        vm_name (str): Name of VM,
        dev (bool): True if dev mode is on, False otherwise

    Returns:
        json: Success/failure
    """

    output = fractalSQLUpdate(
        table_name="v_ms",
        conditional_params={"vm_name": vm_name},
        new_params={"dev": dev},
    )

    if output["success"]:
        return {"status": SUCCESS}
    return {"status": BAD_REQUEST}


def connectionStatusHelper(available, vm_ip, version=None):
    """Stores ping timestamps in the v_ms table and tracks number of hours used

    Args:
        available (bool): True if VM is not being used, False otherwise
        vm_ip (str): VM IP address

    Returns:
        json: Success/failure
    """

    # Retrieve VM data based on VM IP

    vm_info = None

    output = fractalSQLSelect(table_name="v_ms", params={"ip": vm_ip})

    if output["success"] and output["rows"]:
        vm_info = output["rows"][0]["ip"]
    else:
        return {"status": BAD_REQUEST}
    
    # Bypass Winlogon if VM is Linux
    
    if vm_info["os"] == "Linux":
        fractalSQLUpdate(
            table_name="v_ms",
            conditional_params={
                "vm_name": vm_info["vm_name"]
            },
            new_params {
                "ready_to_connect": dateToUnix(getToday())
            }
        )
    
    # Update disk version
    
    if version:
        fractalSQLUpdate(
            table_name="disks",
            conditional_params={
                "disk_name": vm_info["disk_name"]
            },
            new_params={
                "version": version
            }
        )
        
    # Detect and handle disconnect event
    
    if vm_info["state"] == "RUNNING_UNAVAILABLE" and available:
        
        # Add pending charge
        
