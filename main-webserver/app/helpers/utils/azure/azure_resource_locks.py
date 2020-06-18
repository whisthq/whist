from app import *


def lockVMAndUpdate(vm_name, state, lock, temporary_lock):
    """Changes the state, lock, and temporary lock of a VM

    Args:
        vm_name (str): Name of VM
        state (str): Desired state of VM
            [RUNNING_AVAILABLE, RUNNING_UNAVAILABLE, DEALLOCATED, DEALLOCATING, STOPPED, STOPPING, 
            DELETING, CREATING, RESTARTING, STARTING]
        lock (bool): True if VM is locked, False otherwise
        temporary_lock (int): Number of minutes, starting from now, to lock the VM (max is 10)
         

    Returns:
        int: 1 = vm is unlocked, -1 = giving up
    """
    MAX_LOCK_TIME = 10

    new_params = {"state": state, "lock": lock}

    if temporary_lock:
        new_temporary_lock = min(MAX_LOCK_TIME, temporary_lock)
        new_temporary_lock = shiftUnixByMinutes(dateToUnix(getToday()), temporary_lock)

        new_params["temporary_lock"] = new_temporary_lock

    fractalLog(
        function="lockVMAndUpdate",
        label=str(vm_name),
        logs="State: {state}, Lock: {lock}, Temporary Lock: {temporary_lock}".format(
            state=state, lock=str(lock), temporary_lock=str(temporary_lock),
        ),
    )

    output = fractalSQLUpdate(
        table_name="v_ms",
        conditional_params={"vm_name": vm_name},
        new_params=new_params,
    )


def checkLock(vm_name):
    output = fractalSQLSelect(table_name="v_ms", params={"vm_name": vm_name})

    locked = False

    if output["success"] and output["rows"]:
        locked = output["rows"][0]["lock"]

    return locked


def spinLock(vm_name, s=None):
    """Waits for vm to be unlocked

    Args:
        vm_name (str): Name of vm of interest
        ID (int, optional): Unique papertrail logging id. Defaults to -1.

    Returns:
        int: 1 = vm is unlocked, -1 = giving up
    """
    # Check if VM is currently locked

    output = fractalSQLSelect(table_name="v_ms", params={"vm_name": vm_name})

    if output["success"] and output["rows"]:
        username = output["rows"][0]["username"]
    else:
        return -1

    locked = checkLock(vm_mame)
    num_tries = 0

    if not locked:
        fractalLog(
            function="spinLock",
            label=str(username),
            logs="VM {vm_name} found unlocked on first try.".format(vm_name=vm_name),
        )
        return 1
    else:
        fractalLog(
            function="spinLock",
            label=str(username),
            logs="VM {vm_name} found locked on first try. Proceeding to wait...".format(
                vm_name=vm_name
            ),
        )
        if s:
            s.update_state(
                state="PENDING",
                meta={
                    "msg": "Cloud PC is downloading an update. This could take a few minutes."
                },
            )

    while locked:
        time.sleep(5)
        locked = checkLock(vm_name)
        num_tries += 1

        if num_tries > 40:
            fractalLog(
                function="spinLock",
                label=str(username),
                logs="VM {vm_name} locked after waiting 200 seconds. Giving up...".format(
                    vm_name=vm_name
                ),
                level=logging.ERROR,
            )
            return -1

    if s:
        s.update_state(state="PENDING", meta={"msg": "Update successfully downloaded."})

    fractalLog(
        function="spinLock",
        label=str(username),
        logs="After waiting {seconds} seconds, VM {vm_name} is unlocked".format(
            seconds=str(num_tries * 5), vm_name=vm_name
        ),
    )

    return 1
