from .helpers.tests.azure_vm import *

# Define global variable (yes, I know, but this entire test is single-threaded)

pytest.vm_name = None
pytest.disk_name = None

# Start off by deleting all the VM resources in the staging resource group, if there are any


@pytest.mark.vm_serial
def test_delete_vm_initial(input_token, admin_token):
    newLine()

    def deleteVMHelper(vm):

        fractalLog(
            function="test_delete_vm_initial",
            label="vm/delete",
            logs="Starting to delete VM {vm_name}".format(vm_name=vm["vm_name"]),
        )

        resp = deleteVM(
            vm_name=vm["vm_name"],
            delete_disk=True,
            resource_group=RESOURCE_GROUP,
            input_token=input_token,
        )

        task = queryStatus(resp, timeout=10)

        if task["status"] < 1:
            fractalLog(
                function="test_delete_vm_initial",
                label="azure_vm/delete",
                logs=task["output"],
                level=logging.ERROR,
            )
            assert False

    all_vms = fetchCurrentVMs(admin_token)
    fractalJobRunner(deleteVMHelper, all_vms)

    assert True


@pytest.mark.vm_serial
@disabled
def test_vm_create(input_token):
    newLine()

    regions = ["eastus"]

    def createVMInRegion(region):
        fractalLog(
            function="test_vm_create",
            label="vm/create",
            logs="Starting to create a VM in {region}".format(region=region),
        )

        resp = createVM(
            "Standard_NV6_Promo", region, "Linux", RESOURCE_GROUP, input_token
        )

        task = queryStatus(resp, timeout=12.5)

        if task["status"] < 1:
            fractalLog(
                function="test_vm_create",
                label="vm/create",
                logs=task["output"],
                level=logging.ERROR,
            )
            assert False

    fractalJobRunner(createVMInRegion, regions)

    assert True
