from app import *
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
