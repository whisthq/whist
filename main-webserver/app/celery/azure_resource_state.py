from app import *
from app.helpers.utils.azure.azure_general import *
from app.helpers.utils.azure.azure_resource_state_management import *
from app.helpers.utils.azure.azure_resource_locks import *


@celery_instance.task(bind=True)
def startVM(self, vm_name, resource_group=os.getenv("VM_GROUP")):
    """Starts an Azure VM

    Args:
        vm_name (str): The name of the VM
        resource_group (str): The name of the VM's resource group

    Returns:
        json: Success/failure
    """

    if spinLock(vm_name, resource_group) < 0:
        return {"status": REQUEST_TIMEOUT}

    fractalLog(
        function="startVM",
        label=getVMUser(vm_name, resource_group),
        logs="Starting VM {vm_name} in resource group {resource_group}.".format(
            vm_name=vm_name, resource_group=resource_group
        ),
    )

    lockVMAndUpdate(vm_name=vm_name, state="STARTING", lock=True, temporary_lock=3)

    _, compute_client, _ = createClients()

    fractalVMStart(vm_name, resource_group=resource_group, s=self)

    output = fractalSQLSelect(
        table_name=resourceGroupToTable(resource_group), params={"vm_name": vm_name}
    )

    fractalLog(
        function="startVM",
        label=getVMUser(vm_name, resource_group),
        logs="VM {vm_name} successfully started.".format(vm_name=vm_name),
    )

    if output["success"] and output["rows"]:
        return output["rows"][0]


@celery_instance.task(bind=True)
def deallocateVM(self, vm_name, resource_group=os.getenv("VM_GROUP")):
    """Deallocates an Azure VM

    Args:
        vm_name (str): The name of the VM
        resource_group (str): The name of the VM's resource group

    Returns:
        json: Success/failure
    """

    if spinLock(vm_name, resource_group) < 0:
        return {"status": REQUEST_TIMEOUT}

    fractalLog(
        function="deallocateVM",
        label=getVMUser(vm_name, resource_group),
        logs="Deallocating VM {vm_name}.".format(vm_name=vm_name),
    )

    lockVMAndUpdate(vm_name=vm_name, state="DEALLOCATING", lock=True, temporary_lock=3)

    _, compute_client, _ = createClients()

    async_vm_deallocate = compute_client.virtual_machines.deallocate(
        resource_group, vm_name
    )
    async_vm_deallocate.wait()

    output = fractalSQLSelect(
        table_name=resourceGroupToTable(resource_group), params={"vm_name": vm_name}
    )

    lockVMAndUpdate(vm_name=vm_name, state="DEALLOCATED", lock=False, temporary_lock=0)

    fractalLog(
        function="deallocateVM",
        label=getVMUser(vm_name, resource_group),
        logs="VM {vm_name} successfully deallocated.".format(vm_name=vm_name),
    )

    if output["success"] and output["rows"]:
        return output["rows"][0]
