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


def runPowershell(vm_name, command, resource_group, input_token):
    return requests.post(
        (SERVER_URL + "/azure_vm/command"),
        json={
            "vm_name": vm_name,
            "command": command,
            "resource_group": resource_group,
        },
        headers={"Authorization": "Bearer " + input_token},
    )
