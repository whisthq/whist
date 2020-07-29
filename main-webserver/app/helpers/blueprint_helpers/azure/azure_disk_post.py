from app import *
from app.helpers.utils.azure.azure_general import *
from app.celery.azure_resource_creation import *


def createHelper(disk_size, username, location, resource_group):
    output = fractalSQLSelect(
        table_name="disks", params={"username": username, "main": True}
    )

    if output["success"] and output["rows"]:
        # Create Empty Task

        task = createDisk.apply_async([disk_size, username, location, resource_group])
        return {"ID": task.id, "status": ACCEPTED}
    else:
        return {"ID": None, "status": NOT_FOUND}


def stunHelper(using_stun, disk_name):
    fractalLog(
        function="stunHelper",
        label=getDiskUser(disk_name),
        logs="Setting disk {disk_name} to using stun {using_stun}".format(
            disk_name=disk_name, using_stun=str(using_stun)
        ),
    )

    output = fractalSQLUpdate(
        table_name="disk_settings",
        conditional_params={"disk_name": disk_name,},
        new_params={"using_stun": using_stun},
    )

    if output["success"]:
        return {"status": SUCCESS}
    else:
        return {"status": BAD_REQUEST}


def branchHelper(branch, disk_name):
    output = fractalSQLUpdate(
        table_name="disk_settings",
        conditional_params={"disk_name": disk_name,},
        new_params={"branch": branch},
    )

    if output["success"]:
        return {"status": SUCCESS}
    else:
        return {"status": BAD_REQUEST}
