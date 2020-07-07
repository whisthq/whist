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
