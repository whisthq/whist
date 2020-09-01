from app import *
from app.helpers.utils.azure.azure_general import *
from app.celery.azure_resource_creation import *

from app.models.hardware import *
from app.serializers.hardware import *


def createHelper(disk_size, username, location, resource_group, operating_system):
    disk = OSDisk.query.filter_by(user_id=username).first()

    if disk:
        # Create Empty Task

        task = createDisk.apply_async([disk_size, username, location, resource_group, operating_system])
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

    disk = OSDisk.query.get(disk_name)
    disk.using_stun = using_stun

    try:
        db.session.commit()
        return {"status": SUCCESS}
    except Exception:
        return {"status": BAD_REQUEST}


def branchHelper(branch, disk_name):
    disk = OSDisk.query.get(disk_name)
    disk.branch = branch

    try:
        db.session.commit()
        return {"status": SUCCESS}
    except Exception:
        return {"status": BAD_REQUEST}
