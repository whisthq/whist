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

    vm = UserVM.query.filter_by(vm_id=vm_name)
    fractalSQLCommit(db, lambda _, x: x.update({"has_dedicated_vm": dev}), vm)

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

    vm_info = UserVM.query.filter_by(ip=vm_ip).first()
    username = None

    if vm_info:
        username = vm_info.user_id
    else:
        return {"status": BAD_REQUEST}

    fractalLog(function="", label="", logs=str(username))

    disk = OSDisk.query.filter_by(user_id=username).first()

    fractalSQLCommit(
        db, fractalSQLUpdate, disk, {"last_pinged": dateToUnix(getToday())}
    )

    # Update disk version

    if version:
        fractalSQLCommit(db, fractalSQLUpdate, disk, {"version": version})

    # Define states where we don't change the VM state

    intermediate_states = ["STOPPING", "DEALLOCATING", "ATTACHING"]

    # Detect and handle disconnect event

    if vm_info.state == "RUNNING_UNAVAILABLE" and available:

        # Add pending charge if the user is an hourly subscriber

        stripeChargeHourly(username)

        # Add logoff event to timetable

        log = LoginHistory(
            user_id=username,
            action="logoff",
            timestamp=dateToUnix(getToday()),
        )

        fractalSQLCommit(db, lambda db, x: db.session.add(x), log)

        fractalLog(
            function="pingHelper",
            label=str(username),
            logs="{username} just disconnected from their cloud PC".format(
                username=username
            ),
        )

    # Detect and handle logon event

    if vm_info.state == "RUNNING_AVAILABLE" and not available:

        # Add logon event to timetable

        log = LoginHistory(
            user_id=username,
            action="logon",
            timestamp=dateToUnix(getToday()),
        )

        fractalSQLCommit(db, lambda db, x: db.session.add(x), log)

        fractalLog(
            function="pingHelper",
            label=str(username),
            logs="{username} just connected to their cloud PC".format(
                username=username
            ),
        )

    # Change VM states accordingly

    if not vm_info.state in intermediate_states and not available:
        lockVMAndUpdate(
            vm_name=vm_info.vm_id,
            state="RUNNING_UNAVAILABLE",
            lock=True,
            temporary_lock=0,
        )

    if not vm_info.state in intermediate_states and available:
        lockVMAndUpdate(
            vm_name=vm_info.vm_id,
            state="RUNNING_AVAILABLE",
            lock=False,
            temporary_lock=None,
        )

    return {"status": SUCCESS}
