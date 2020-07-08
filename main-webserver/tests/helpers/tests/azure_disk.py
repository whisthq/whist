from tests import *


def fetchCurrentDisks():
    output = fractalSQLSelect(table_name="disks", params={})
    return output["rows"]


def deleteDisk(disk_name, resource_group, input_token):
    return requests.post(
        (SERVER_URL + "/azure_disk/delete"),
        json={"disk_name": disk_name, "resource_group": resource_group},
        headers={"Authorization": "Bearer " + input_token},
    )


def cloneDisk(location, vm_size, operating_system, apps, resource_group, input_token):
    return requests.post(
        (SERVER_URL + "/azure_disk/clone"),
        json={
            "username": "fakefake{location}@gmail.com".format(location=location),
            "location": location,
            "vm_size": vm_size,
            "operating_system": operating_system,
            "apps": apps,
            "resource_group": resource_group,
        },
        headers={"Authorization": "Bearer " + input_token},
    )


def attachDisk(disk_name, resource_group, vm_name=None):
    return requests.post(
        (SERVER_URL + "/azure_disk/attach"),
        json={
            "disk_name": disk_name,
            "resource_group": resource_group,
            "vm_name": vm_name,
        },
        headers={"Authorization": "Bearer " + input_token},
    )


def createDisk(location, disk_size, resource_group, input_token):
    return requests.post(
        (SERVER_URL + "/azure_disk/create"),
        json={
            "username": "fakefake{location}@gmail.com".format(location=location),
            "location": location,
            "disk_size": disk_size,
            "resource_group": resource_group,
        },
        headers={"Authorization": "Bearer " + input_token},
    )
