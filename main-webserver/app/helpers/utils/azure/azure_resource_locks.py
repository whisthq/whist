from app import *


def lockVMAndUpdate(vm_name, state, lock, temporary_lock):
    MAX_LOCK_TIME = 10

    new_params = {"state": state, "lock": lock}

    if temporary_lock:
        new_temporary_lock = min(MAX_LOCK_TIME, temporary_lock)
        new_temporary_lock = shiftUnixByMinutes(dateToUnix(getToday()), temporary_lock)

        new_params["temporary_lock"] = new_temporary_lock

    fractalLog(
        function="lockVMAndUpdate",
        label="VM {vm_name}".format(vm_name),
        logs="State: {state}, Lock: {lock}, Temporary Lock: {temporary_lock}".format(
            state=state, lock=str(lock), temporary_lock=str(temporary_lock),
        ),
    )

    output = fractalSQLUpdate(
        table_name="v_ms",
        conditional_params={"vm_name": vm_name},
        new_params=new_params,
    )
