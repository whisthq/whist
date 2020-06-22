from app import *
from app.helpers.utils.azure.azure_general import *
from app.helpers.utils.azure.azure_resource_creation import *
from app.helpers.utils.azure.azure_resource_state_management import *
from app.helpers.utils.azure.azure_resource_locks import *


@celery_instance.task(bind=True)
def swapSpecificDisk(self, vm_name, disk_name, resource_group=None):
    """Swaps out a disk in a vm

    Args:
        disk_name (str): The name of the disk to swap out
        vm_name (str): The name of the vm the disk is attached to
        ID (int, optional): Papertrail logging ID. Defaults to -1.

    Returns:
        json: VM info
    """

    fractalLog(
        function="swapSpecificDisk",
        label=str(vm_name),
        logs="Beginning function to swap disk {disk_name} into VM {vm_name}".format(
            disk_name=disk_name, vm_name=vm_name
        ),
    )

    resource_group = os.getenv("VM_GROUP") if not resource_group else resource_group

    if spinLock(vm_name, resource_group=resource_group) < 0:
        return {"status": REQUEST_TIMEOUT, "payload": None}

    lockVMAndUpdate(vm_name=vm_name, state="ATTACHING", lock=True, temporary_lock=3)

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

    fractalVMStart(vm_name, s=self)

    lockVMAndUpdate(
        vm_name=vm_name, state="RUNNING_AVAILABLE", lock=False, temporary_lock=0
    )

    output = fractalSQLSelect(
        table_name=resourceGroupToTable(resource_group), params={"vm_name": vm_name}
    )

    return {"status": SUCCESS, "payload": output["rows"][0]}


@celery_instance.task(bind=True)
def deployArtifact(
    self, vm_name, artifact_name, run_id, resource_group="FractalProtocolCI"
):
    """Swaps out a disk in a vm

    Args:
        vm_name (str): The name of the vm
        artifact_name (str): Name of artifact
        run_id (str): Run ID

    Returns:
        json: Success/failure
    """

    _, compute_client, _ = createClients()

    with open(
        os.path.join(app.config["ROOT_DIRECTORY"], "scripts/deployArtifact.txt"), "r"
    ) as file:
        fractalLog(
            function="deployArtifact",
            label=str(vm_name),
            logs="Starting to run deploy artifact script",
        )

        command = (
            file.read()
            .replace("ARTIFACT_NAME", artifact_name)
            .replace("RUN_ID", str(run_id))
        )
        run_command_parameters = {
            "command_id": "RunPowerShellScript",
            "script": [command],
        }

        poller = compute_client.virtual_machines.run_command(
            resource_group, vm_name, run_command_parameters
        )

        result = poller.result()

        fractalLog(
            function="deployArtifact",
            label=str(vm_name),
            logs="Artifact script finished running. Output: {output}".format(
                output=str(result.value[0])
            ),
        )

    return {"status": SUCCESS, "payload": str(result.value[0])}
