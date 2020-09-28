from app import db
from app.constants.http_codes import BAD_REQUEST, SUCCESS
from app.helpers.utils.azure.azure_general import getVMUser
from app.helpers.utils.general.logs import fractalLog
from app.helpers.utils.general.sql_commands import fractalSQLCommit
from app.models.hardware import UserVM


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
        logs="Setting VM {vm_name} dev mode to {dev}".format(vm_name=str(vm_name), dev=str(dev)),
    )

    vm = UserVM.query.filter_by(vm_id=vm_name)
    if fractalSQLCommit(db, lambda _, x: x.update({"has_dedicated_vm": dev}), vm):
        return {"status": SUCCESS}
    else:
        return {"status": BAD_REQUEST}
