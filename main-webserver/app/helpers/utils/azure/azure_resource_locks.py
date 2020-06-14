from app import *


def lockVMAndUpdate(vm_name, state, lock, temporary_lock):
    MAX_LOCK_TIME = 10

    new_params = {"state": state, "lock": lock}

    if temporary_lock:
        temporary_lock = min(MAX_LOCK_TIME, temporary_lock)
        temporary_lock = shiftUnixByMinutes(dateToUnix(getToday()), temporary_lock)

        new_params["temporary_lock"] = temporary_lock

    fractalSQLUpdate(
        table_name="v_ms",
        conditional_params={"vm_name": "vm_name"},
        new_params={"state": state, "lock": lock,},
    )
