from app import *
from app.helpers.utils.azure.azure_resource_locks import *
from app.helpers.utils.stripe.stripe_payments import *


def devHelper(vm_name, dev):
    """Toggles the dev mode of a VM

    Args:
        vm_name (str): Name of VM,
        dev (bool): True if dev mode is on, False otherwise

    Returns:
        json: Success/failure
    """

    fractalLog(
        function="pingHelper",
        label=getVMUser(vm_name),
        logs="Setting VM {vm_name} dev mode to {dev}".format(
            vm_name=str(vm_name), dev=str(dev)
        ),
    )

    output = fractalSQLUpdate(
        table_name="v_ms",
        conditional_params={"vm_name": vm_name},
        new_params={"dev": dev},
    )

    if output["success"]:
        return {"status": SUCCESS}
    return {"status": BAD_REQUEST}


def pingHelper(available, vm_ip, version=None):
    """Stores ping timestamps in the v_ms table and tracks number of hours used

    Args:
        available (bool): True if VM is not being used, False otherwise
        vm_ip (str): VM IP address

    Returns:
        json: Success/failure
    """

    # Retrieve VM data based on VM IP

    vm_info = None
    username = None

    output = fractalSQLSelect(table_name="v_ms", params={"ip": vm_ip})

    if output["success"] and output["rows"]:
        vm_info = output["rows"][0]
        username = vm_info["username"]
    else:
        return {"status": BAD_REQUEST}

    fractalSQLUpdate(
        table_name="v_ms",
        conditional_params={"vm_name": vm_info["vm_name"]},
        new_params={"ready_to_connect": dateToUnix(getToday())},
    )

    # Update disk version

    if version:
        fractalSQLUpdate(
            table_name="disks",
            conditional_params={"disk_name": vm_info["disk_name"]},
            new_params={"version": version},
        )

    # Define states where we don't change the VM state

    intermediate_states = ["STOPPING", "DEALLOCATING", "ATTACHING"]

    # Detect and handle disconnect event

    if vm_info["state"] == "RUNNING_UNAVAILABLE" and available:

        # Add pending charge if the user is an hourly subscriber

        stripeChargeHourly(username)

        # Add logoff event to timetable

        fractalSQLInsert(
            table_name="login_history",
            params={
                "username": username,
                "timestamp": dt.now().strftime("%m-%d-%Y, %H:%M:%S"),
                "action": "logoff",
            },
        )

        fractalLog(
            function="pingHelper",
            label=str(username),
            logs="{username} just disconnected from their cloud PC".format(
                username=username
            ),
        )

    # Detect and handle logon event

    if vm_info["state"] == "RUNNING_AVAILABLE" and not available:

        # Add logon event to timetable

        fractalSQLInsert(
            table_name="login_history",
            params={
                "username": username,
                "timestamp": dt.now().strftime("%m-%d-%Y, %H:%M:%S"),
                "action": "logon",
            },
        )

        fractalLog(
            function="pingHelper",
            label=str(username),
            logs="{username} just connected to their cloud PC".format(
                username=username
            ),
        )

    # Change VM states accordingly

    if not vm_info["state"] in intermediate_states and not available:
        lockVMAndUpdate(
            vm_name=vm_info["vm_name"],
            state="RUNNING_UNAVAILABLE",
            lock=True,
            temporary_lock=0,
        )

    if not vm_info["state"] in intermediate_states and available:
        lockVMAndUpdate(
            vm_name=vm_info["vm_name"],
            state="RUNNING_AVAILABLE",
            lock=False,
            temporary_lock=None,
        )

    return {"status": SUCCESS}
