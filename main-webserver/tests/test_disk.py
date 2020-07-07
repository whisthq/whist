from .helpers.tests.azure_disk import *


# Start off by deleting all the disks in the staging resource group, if there are any


def test_delete_disk_initial(input_token):
    if RESOURCE_GROUP == "FractalStaging":
        all_disks = fetchCurrentDisks()

        if all_disks:
            fractalLog(
                function="test_delete_disk_initial",
                label="None",
                logs="Found {num_disks} in database. Starting to delete...".format(
                    num_disks=str(len(all_disks))
                ),
            )

            for disk in all_disks:
                resp = deleteDisk(
                    disk_name=disk["disk_name"],
                    resource_group=RESOURCE_GROUP,
                    input_token=input_token,
                )

                if queryStatus(resp) < 1:
                    assert False
        else:
            fractalLog(
                function="test_delete_disk_initial",
                label="None",
                logs="No disks found in database",
            )
        assert True
    else:
        fractalLog(
            function="test_delete_disk_initial",
            label="None",
            logs="Resouce group is not staging resource group! Forcefully stopping tests.",
            level=logging.WARNING,
        )

        assert False


def test_disk_clone(input_token):
    resp = cloneDisk(
        username=genHaiku(1)[0],
        location="eastus",
        vm_size="NV6",
        operating_system="Windows",
        apps=[],
        resource_group=RESOURCE_GROUP,
        input_token=input_token,
    )

    if queryStatus(resp) < 1:
        assert False

