from app import *
from app.helpers.utils.azure.azure_general import *


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
        fractaLog(
            function="attachDiskToVM",
            label=getVMUser(vm_name),
            log="Critical error attaching disk to VM: {error}".format(error=error),
            level=logging.CRITICAL,
        )

        return -1
