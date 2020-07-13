from app import *
from app.helpers.utils.azure.azure_general import *
from app.helpers.utils.azure.azure_helpers.azure_resource_modification_helpers import *


def attachDiskToVM(disk_name, vm_name, resource_group=os.getenv("VM_GROUP")):
    """Creates a network id

    Args:
        name (str): Name of the vm
        location (str): The azure region
        tries (int): The current number of tries

    Returns:
        dict: The network id object
    """

    try:
        _, compute_client, _ = createClients()
        new_os_disk = compute_client.disks.get(resource_group, disk_name)

        vm = createVMInstance(vm_name, resource_group=resource_group)
        vm.storage_profile.os_disk.managed_disk.id = new_os_disk.id
        vm.storage_profile.os_disk.name = new_os_disk.name

        fractalLog(
            function="swapSpecificDisk",
            label=str(vm_name),
            logs="Sending Azure command to swap disk {disk_name} into VM {vm_name}".format(
                disk_name=disk_name, vm_name=vm_name
            ),
        )

        async_disk_attach = compute_client.virtual_machines.create_or_update(
            resource_group, vm.name, vm
        )
        async_disk_attach.wait()

        fractalLog(
            function="swapSpecificDisk",
            label=str(vm_name),
            logs="Disk {disk_name} successfully swapped onto VM {vm_name}".format(
                disk_name=disk_name, vm_name=vm_name
            ),
        )

        output = fractalSQLSelect(table_name="disks", params={"disk_name": disk_name})

        username = None
        if output["success"] and output["rows"]:
            username = output["rows"][0]["username"]

        if output["success"] and output["rows"]:
            fractalSQLUpdate(
                table_name=resourceGroupToTable(resource_group),
                conditional_params={"vm_name": vm_name},
                new_params={"disk_name": disk_name, "username": str(username)},
            )

        return 1
    except Exception as e:
        fractalLog(
            function="attachDiskToVM",
            label=getVMUser(vm_name),
            log="Critical error attaching disk to VM: {error}".format(error=error),
            level=logging.CRITICAL,
        )

        return -1


def detachSecondaryDisk(disk_name, vm_name, resource_group, s=None):
    """Detaches disk from vm

    Args:
        vm_name (str): Name of the vm
        disk_name (str): Name of the disk
        ID (int, optional): Papertrail logging ID. Defaults to -1.
    """
    _, compute_client, _ = createClients()
    # Detach data disk
    fractalLog(
        function="detachSecondaryDisk",
        label=getVMUser(vm_name),
        logs="Detaching secondary disk {disk_name} to {vm_name}".format(
            disk_name=disk_name, vm_name=vm_name
        ),
    )

    virtual_machine = createVMInstance(vm_name, resource_group)

    data_disks = virtual_machine.storage_profile.data_disks
    data_disks[:] = [disk for disk in data_disks if disk.name != disk_name]
    async_vm_update = compute_client.virtual_machines.create_or_update(
        resource_group, vm_name, virtual_machine
    )
    virtual_machine = async_vm_update.result()
    return 1
