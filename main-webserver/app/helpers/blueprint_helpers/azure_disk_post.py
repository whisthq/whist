from app import *
from app.celery.azure_resource_creation import *


def createHelper(disk_size, username):
    output = fractalSQLSelect(
        table_name="disks", params={"username": username, "main": True}
    )

    if output["success"] and output["rows"]:
        # Create Empty Task
        return {"ID": None, "status": ACCEPTED}
    else:
        return {"ID": None, "status": BAD_REQUEST}
