from .helpers.tests.azure_disk import *
from .helpers.tests.azure_vm import *

# Start off by deleting all the disks in the staging resource group, if there are any


@pytest.mark.disk_serial
@disabled
def test_delete_disk_initial(input_token, admin_token):
    if os.getenv("USE_PRODUCTION_DATABASE").upper() == "TRUE":
        fractalLog(
            function="test_delete_disk_initial",
            label="azure_disk/delete",
            logs="Using production database instead of staging database! Forcefully stopping tests.",
            level=logging.WARNING,
        )

        assert False

    if RESOURCE_GROUP == "FractalStaging":

        def deleteDiskHelper(disk):
            fractalLog(
                function="test_delete_disk_initial",
                label="azure_disk/delete",
                logs="Deleting disk {disk_name}".format(disk_name=disk["disk_name"]),
            )

            resp = deleteDisk(
                disk_name=disk["disk_name"],
                resource_group=RESOURCE_GROUP,
                input_token=input_token,
            )

            task = queryStatus(resp, timeout=2)

            if task["status"] < 1:
                fractalLog(
                    function="test_delete_disk_initial",
                    label="azure_disk/delete",
                    logs=task["output"],
                    level=logging.ERROR,
                )
                assert False

        all_disks = fetchCurrentDisks(admin_token)

        if all_disks:
            fractalLog(
                function="test_delete_disk_initial",
                label="azure_disk/delete",
                logs="Found {num_disks} disk(s) already in database. Starting to delete...".format(
                    num_disks=str(len(all_disks))
                ),
            )

            fractalJobRunner(deleteDiskHelper, all_disks)

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


@pytest.mark.disk_serial
@disabled
def test_disk_clone(input_token):
    regions = ["eastus", "eastus", "eastus"]

    def cloneDiskHelper(region):
        fractalLog(
            function="test_disk_clone",
            label="azure_disk/clone",
            logs="Starting to clone a disk in {region}".format(region=region),
        )

        resp = cloneDisk(
            location=region,
            vm_size="NV6",
            operating_system="Windows",
            apps=[],
            resource_group=RESOURCE_GROUP,
            input_token=input_token,
        )

        task = queryStatus(resp, timeout=1.2)

        if task["status"] < 1:
            fractalLog(
                function="test_disk_clone",
                label="azure_disk/clone",
                logs=task["output"],
                level=logging.ERROR,
            )
            assert False

    fractalJobRunner(cloneDiskHelper, regions)

    assert True


# Test disk attaching


@pytest.mark.disk_serial
def test_disk_attach(input_token, admin_token):
    regions = ["eastus", "northcentralus", "southcentralus"]

    def attachDiskHelper(disk):
        disk_name = disk["disk_name"]
        if disk["main"] and disk["state"] == "ACTIVE":
            fractalLog(
                function="test_disk_attach",
                label="azure_disk/attach",
                logs="Starting to attach disk {disk_name} to an available VM".format(
                    disk_name=disk_name
                ),
            )

            resp = attachDisk(
                disk_name=disk_name,
                resource_group=RESOURCE_GROUP,
                input_token=input_token,
            )

            task = queryStatus(resp, timeout=8)

            if task["status"] < 1:
                fractalLog(
                    function="test_disk_attach",
                    label="azure_disk/attach",
                    logs=task["output"],
                    level=logging.ERROR,
                )
                assert False

            fractalLog(
                function="test_disk_attach",
                label="azure_disk/attach",
                logs="Running powershell script on {disk_name}".format(
                    disk_name=disk_name
                ),
            )

            resp = runPowershell(
                vm_name="shinymode749971",
                command="choco install firefox --force",
                resource_group=RESOURCE_GROUP,
                input_token=input_token,
            )

            task = queryStatus(resp, timeout=4)

            if task["status"] < 1:
                fractalLog(
                    function="test_disk_attach",
                    label="azure_disk/attach",
                    logs=task["output"],
                    level=logging.ERROR,
                )
                assert False

    disks = fetchCurrentDisks(admin_token)
    fractalJobRunner(attachDiskHelper, disks, multithreading=False)

    assert True


@pytest.mark.disk_serial
@disabled
def test_disk_create(input_token):
    region = "eastus"

    fractalLog(
        function="test_disk_create",
        label="azure_disk/create",
        logs="Starting to create a disk in {region}".format(region=region),
    )

    resp = createDisk(
        location=region,
        disk_size=127,
        resource_group=RESOURCE_GROUP,
        input_token=input_token,
    )

    task = queryStatus(resp, timeout=1.2)

    if task["status"] < 1:
        fractalLog(
            function="test_disk_create",
            label="azure_disk/create",
            logs=task["output"],
            level=logging.ERROR,
        )
        assert False
