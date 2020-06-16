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
