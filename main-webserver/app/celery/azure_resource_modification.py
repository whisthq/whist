from app import *
from app.helpers.utils.azure.azure_general import *
from app.helpers.utils.azure.azure_resource_creation import *
from app.helpers.utils.azure.azure_resource_state_management import *
from app.helpers.utils.azure.azure_resource_locks import *
from app.helpers.utils.azure.azure_resource_modification import *


@celery_instance.task(bind=True)
def swapSpecificDisk(self, vm_name, disk_name, resource_group=os.getenv("VM_GROUP")):
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

    if spinLock(vm_name, resource_group=resource_group) < 0:
        return {"status": REQUEST_TIMEOUT, "payload": None}

    lockVMAndUpdate(vm_name=vm_name, state="ATTACHING", lock=True, temporary_lock=3)

    attachDiskToVM(disk_name, vm_name, resource_group)

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


@celery_instance.task(bind=True)
def automaticAttachDisk(self, disk_name, resource_group=os.getenv("VM_GROUP")):
    """Find an available VM to attach a disk to

    Args:
        disk_name (str): The name of the disk to swap out

    Returns:
        json:
    """

    sendInfo(ID, " Swap disk task for disk {} added to Redis queue".format(disk_name))

    # Get the
    _, compute_client, _ = createClients()

    os_disk = compute_client.disks.get(os.environ.get("VM_GROUP"), disk_name)
    os_type = "Windows" if "windows" in str(os_disk.os_type) else "Linux"
    needs_winlogon = os_type == "Windows"
    username = mapDiskToUser(disk_name)
    vm_name = os_disk.managed_by

    location = os_disk.location
    vm_attached = True if vm_name else False

    if vm_attached:
        self.update_state(
            state="PENDING",
            meta={
                "msg": "Boot request received successfully. Preparing your cloud PC."
            },
        )
        sendInfo(
            ID,
            " Azure says that disk {} belonging to {} is attached to {}".format(
                disk_name, username, vm_name
            ),
        )
    else:
        self.update_state(
            state="PENDING",
            meta={"msg": "Boot request received successfully. Fetching your cloud PC."},
        )
        sendInfo(
            ID,
            " Azure says that disk {} belonging to {} is not attached to any VM".format(
                disk_name, username
            ),
        )

    # Update the database to reflect the disk attached to the VM currently
    if vm_attached:
        vm_name = vm_name.split("/")[-1]
        sendInfo(ID, "{}is attached to {}".format(username, vm_name))

        unlocked = False
        while not unlocked and vm_attached:
            if spinLock(vm_name, s=self) > 0:
                unlocked = True
                # Lock immediately
                lockVMAndUpdate(
                    vm_name=vm_name,
                    state="ATTACHING",
                    lock=True,
                    temporary_lock=None,
                    change_last_updated=True,
                    verbose=False,
                    ID=ID,
                )
                lockVM(vm_name, True, username=username, disk_name=disk_name, ID=ID)

                # Update database with new disk name and VM state
                sendInfo(
                    ID,
                    " Disk {} belongs to user {} and is already attached to VM {}".format(
                        disk_name, username, vm_name
                    ),
                )
                updateDisk(disk_name, vm_name, location)

                self.update_state(
                    state="PENDING",
                    meta={
                        "msg": "Database updated. Sending signal to boot your cloud PC."
                    },
                )

                sendInfo(
                    ID, " Database updated with {} and {}".format(disk_name, vm_name)
                )

                if fractalVMStart(vm_name, needs_winlogon=needs_winlogon, s=self) > 0:
                    sendInfo(ID, " VM {} is started and ready to use".format(vm_name))
                    self.update_state(
                        state="PENDING", meta={"msg": "Cloud PC is ready to use."}
                    )
                else:
                    sendError(ID, " Could not start VM {}".format(vm_name))
                    self.update_state(
                        state="FAILURE",
                        meta={
                            "msg": "Cloud PC could not be started. Please contact support."
                        },
                    )

                vm_credentials = fetchVMCredentials(vm_name)

                attachSecondaryDisks(self, username, vm_name)

                lockVMAndUpdate(
                    vm_name=vm_name,
                    state="RUNNING_AVAILABLE",
                    lock=False,
                    temporary_lock=1,
                    change_last_updated=True,
                    verbose=False,
                    ID=ID,
                )

                return vm_credentials
            else:
                os_disk = compute_client.disks.get(
                    os.environ.get("VM_GROUP"), disk_name
                )
                vm_name = os_disk.managed_by
                location = os_disk.location
                vm_attached = True if vm_name else False

    if not vm_attached:
        disk_attached = False
        while not disk_attached:
            vm = claimAvailableVM(disk_name, location, os_type, s=self)
            if vm:
                try:
                    vm_name = vm["vm_name"]

                    vm_info = compute_client.virtual_machines.get(
                        os.getenv("VM_GROUP"), vm_name
                    )

                    for disk in vm_info.storage_profile.data_disks:
                        if disk.name != disk_name:
                            self.update_state(
                                state="PENDING",
                                meta={
                                    "msg": "Making sure that you have a stable connection."
                                },
                            )

                            sendInfo(
                                ID,
                                "Detaching disk {} from {}".format(disk_name, vm_name),
                            )

                            detachDisk(disk.name, vm_name)

                    sendInfo(
                        ID,
                        "Disk {} was unattached. VM {} claimed for {}".format(
                            disk_name, vm_name, username
                        ),
                    )

                    if swapDiskAndUpdate(self, disk_name, vm_name, needs_winlogon) > 0:
                        self.update_state(
                            state="PENDING",
                            meta={"msg": "Data successfully uploaded to cloud PC."},
                        )
                        free_vm_found = True
                        updateOldDisk(vm_name)
                        attachSecondaryDisks(self, username, vm_name)

                        lockVMAndUpdate(
                            vm_name=vm_name,
                            state="RUNNING_AVAILABLE",
                            lock=False,
                            temporary_lock=1,
                            change_last_updated=True,
                            verbose=False,
                            ID=ID,
                        )

                        return fetchVMCredentials(vm_name)

                    vm_credentials = fetchVMCredentials(vm_name)
                    attachSecondaryDisks(self, username, vm_name)

                    lockVMAndUpdate(
                        vm_name=vm_name,
                        state="RUNNING_AVAILABLE",
                        lock=False,
                        temporary_lock=1,
                        change_last_updated=True,
                        verbose=False,
                        ID=ID,
                    )

                    disk_attached = True
                    sendInfo(
                        ID,
                        " VM {} successfully attached to disk {}".format(
                            vm_name, disk_name
                        ),
                    )

                    return vm_credentials
                except Exception as e:
                    sendCritical(ID, str(e))

            else:
                self.update_state(
                    state="PENDING",
                    meta={
                        "msg": "Running performance tests. This could take a few extra minutes."
                    },
                )
                sendInfo(
                    ID,
                    "No VMs are available for {} using {}. Going to sleep...".format(
                        username, disk_name
                    ),
                )
                time.sleep(30)

    self.update_state(
        state="FAILURE",
        meta={"msg": "Cloud PC could not be started. Please contact support."},
    )
    return None
