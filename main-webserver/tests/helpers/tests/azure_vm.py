from tests import *


# Get all the VMs in the database currently


def fetchCurrentVMs():
    output = fractalSQLSelect(table_name="v_ms", params={})
    return output["rows"]


def createVM(vm_size, location, operating_system, resource_group, input_token):
    return requests.post(
        (SERVER_URL + "/azure_vm/create"),
        json={
            "vm_size": vm_size,
            "location": location,
            "operating_system": operating_system,
            "resource_group": resource_group,
        },
        headers={"Authorization": "Bearer " + input_token},
    )


def getVm(vm_name):
    resp = requests.post((SERVER_URL + "/vm/fetchVm"), json={"vm_name": vm_name})
    return resp.json()["vm"]


def deleteVM(vm_name, delete_disk, resource_group, input_token):
    return requests.post(
        (SERVER_URL + "/azure_vm/delete"),
        json={
            "vm_name": vm_name,
            "delete_disk": delete_disk,
            "resource_group": resource_group,
        },
        headers={"Authorization": "Bearer " + input_token},
    )


def createDiskFromImage(
    operating_system, username, location, vm_size, apps, input_token
):
    return requests.post(
        (SERVER_URL + "/disk/createFromImage"),
        json={
            "operating_system": operating_system,
            "username": username,
            "location": location,
            "vm_size": vm_size,
            "apps": apps,
            "resource_group": os.getenv("VM_GROUP"),
        },
        headers={"Authorization": "Bearer " + input_token},
    )


def swap(vm_name, disk_name, input_token):
    return requests.post(
        (SERVER_URL + "/disk/swap"),
        json={"vm_name": vm_name, "disk_name": disk_name},
        headers={"Authorization": "Bearer " + input_token},
    )
