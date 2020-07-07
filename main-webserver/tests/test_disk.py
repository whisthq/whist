from .helpers.tests.azure_disk import *
from .helpers.tests.azure_vm import *

# Start off by deleting all the disks in the staging resource group, if there are any


def test_delete_disk_initial(input_token):
    if os.getenv("USE_PRODUCTION_DATABASE").upper() == "TRUE":
        fractalLog(
            function="test_delete_disk_initial",
            label="azure_disk/delete",
            logs="Using production database instead of staging database! Forcefully stopping tests.",
            level=logging.WARNING,
        )

        assert False
    if RESOURCE_GROUP == "FractalStaging":
        all_disks = fetchCurrentDisks()

        if all_disks:
            fractalLog(
                function="test_delete_disk_initial",
                label="azure_disk/delete",
                logs="Found {num_disks} disk already in database. Starting to delete...".format(
                    num_disks=str(len(all_disks))
                ),
            )

            for disk in all_disks:
                resp = deleteDisk(
                    disk_name=disk["disk_name"],
                    resource_group=RESOURCE_GROUP,
                    input_token=input_token,
                )

                if queryStatus(resp, timeout=2) < 1:
                    assert False
        else:
            fractalLog(
                function="test_delete_disk_initial",
                label="azure_disk/delete",
                logs="No disks found in database",
            )
        assert True
    else:
        fractalLog(
            function="test_delete_disk_initial",
            label="azure_disk/delete",
            logs="Resouce group is not staging resource group! Forcefully stopping tests.",
            level=logging.WARNING,
        )

        assert False


# Test disk cloning in each Azure region


def test_disk_clone(input_token):
    regions = ["eastus", "southcentralus", "northcentralus"]

    for region in regions:
        fractalLog(
            function="test_delete_disk_initial",
            label="azure_disk/clone",
            logs="Starting to clone a disk in {region}".format(region=region),
        )

        resp = cloneDisk(
            username=genHaiku(1)[0],
            location=region,
            vm_size="NV6",
            operating_system="Windows",
            apps=[],
            resource_group=RESOURCE_GROUP,
            input_token=input_token,
        )

        if queryStatus(resp, timeout=1.2) < 1:
            assert False

    assert True


# Test disk attaching


def test_disk_attach(input_token):
    disks = fetchCurrentDisks()
    vms = fetchCurrentVMs()

    for disk in disks:
        
