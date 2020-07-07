from .helpers.tests.azure_disk import *


# Start off by deleting all the VM resources in the staging resource group, if there are any


def test_delete_disk_initial(input_token):
    all_disks = fetchCurrentDisks()
    for disk in all_disks:
        resp = deleteDisk(
            disk_name=disk["disk_name"],
            resource_group=RESOURCE_GROUP,
            input_token=input_token,
        )

        if queryStatus(resp) < 1:
            assert False

    assert True
