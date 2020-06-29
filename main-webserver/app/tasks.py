from app import *
from msrest.exceptions import ClientException
from .logger import *
from .helpers.general import *
from .helpers.vms import *
from .helpers.logins import *
from .helpers.disks import *
from .helpers.s3 import *


@celery.task(bind=True)
def createVM(
    self,
    vm_size,
    location,
    operating_system,
    admin_password=None,
    admin_username=None,
    ID=-1,
):
    """Creates a windows vm of size vm_size in Azure region location

    Args:
        vm_size (str): The size of the vm to create
        location (str): The Azure region

    Returns:
        dict: The dict representing the vm in the v_ms sql table
    """
    sendInfo(
        ID,
        "Creating VM of size {}, location {}, operating system {}".format(
            vm_size, location, operating_system
        ),
    )

    self.update_state(
        state="PENDING",
        meta={
            "msg": "Creating VM of size {}, location {}, operating system {}".format(
                vm_size, location, operating_system
            )
        },
    )

    _, compute_client, _ = createClients()
    vmName = genVMName()
    nic = createNic(vmName, location, 0)
    if not nic:
        sendError(ID, "Nic does not exist, aborting")
        return
    vmParameters = createVMParameters(
        vmName,
        nic.id,
        vm_size,
        location,
        operating_system,
        admin_password=admin_password,
        admin_username=admin_username,
    )

    async_vm_creation = compute_client.virtual_machines.create_or_update(
        os.environ["VM_GROUP"], vmParameters["vm_name"], vmParameters["params"]
    )

    self.update_state(
        state="PENDING", meta={"msg": "Waiting on async VM creation"},
    )

    sendDebug(ID, "Waiting on async_vm_creation")
    async_vm_creation.wait()

    self.update_state(
        state="PENDING", meta={"msg": "VM {} created".format(vmParameters["vm_name"])},
    )

    time.sleep(10)

    self.update_state(
        state="PENDING", meta={"msg": "VM {} starting".format(vmParameters["vm_name"])},
    )

    async_vm_start = compute_client.virtual_machines.start(
        os.environ["VM_GROUP"], vmParameters["vm_name"]
    )
    sendDebug(ID, "Waiting on async_vm_start")
    async_vm_start.wait()

    self.update_state(
        state="PENDING", meta={"msg": "VM {} started".format(vmParameters["vm_name"])},
    )

    time.sleep(30)

    sendInfo(ID, "The VM created is called {}".format(vmParameters["vm_name"]))

    fractalVMStart(vmParameters["vm_name"], needs_winlogon=False, s=self)

    lockVMAndUpdate(
        vm_name=vmParameters["vm_name"],
        state="RUNNING_AVAILABLE",
        lock=True,
        temporary_lock=None,
        change_last_updated=False,
        verbose=False,
    )

    extension_parameters = (
        {
            "location": location,
            "publisher": "Microsoft.HpcCompute",
            "vm_extension_name": "NvidiaGpuDriverWindows",
            "virtual_machine_extension_type": "NvidiaGpuDriverWindows",
            "type_handler_version": "1.2",
        }
        if operating_system == "Windows"
        else {
            "location": location,
            "publisher": "Microsoft.HpcCompute",
            "vm_extension_name": "NvidiaGpuDriverLinux",
            "virtual_machine_extension_type": "NvidiaGpuDriverLinux",
            "type_handler_version": "1.2",
        }
    )

    self.update_state(
        state="PENDING",
        meta={
            "msg": "VM {} installing NVIDIA extension".format(vmParameters["vm_name"])
        },
    )

    async_vm_extension = compute_client.virtual_machine_extensions.create_or_update(
        os.environ["VM_GROUP"],
        vmParameters["vm_name"],
        extension_parameters["vm_extension_name"],
        extension_parameters,
    )

    sendDebug(ID, "Waiting on async_vm_extension")
    async_vm_extension.wait()

    self.update_state(
        state="PENDING",
        meta={
            "msg": "VM {} successfully installed NVIDIA extension".format(
                vmParameters["vm_name"]
            )
        },
    )

    vm = getVM(vmParameters["vm_name"])
    vm_ip = getIP(vm)
    updateVMIP(vmParameters["vm_name"], vm_ip)
    updateVMState(vmParameters["vm_name"], "RUNNING_AVAILABLE")
    updateVMLocation(vmParameters["vm_name"], location)
    updateVMOS(vmParameters["vm_name"], operating_system)

    sendInfo(ID, "SUCCESS: VM {} created and updated".format(vmName))

    lockVMAndUpdate(
        vm_name=vmParameters["vm_name"],
        state="RUNNING_AVAILABLE",
        lock=False,
        temporary_lock=0,
        change_last_updated=False,
        verbose=False,
    )

    return fetchVMCredentials(vmParameters["vm_name"])


@celery.task(bind=True)
def createEmptyDisk(self, disk_size, username, location, ID=-1):
    _, compute_client, _ = createClients()
    disk_name = genDiskName()

    # Create managed data disk
    sendInfo(ID, "Creating (empty) managed Disk: " + disk_name)
    async_disk_creation = compute_client.disks.create_or_update(
        os.environ.get("VM_GROUP"),
        disk_name,
        {
            "location": location,
            "disk_size_gb": disk_size,
            "creation_data": {"create_option": DiskCreateOption.empty},
        },
    )
    data_disk = async_disk_creation.result()

    vm_name = ""
    createDiskEntry(
        disk_name, vm_name, username, location, disk_size=disk_size, main=False
    )

    return disk_name


@celery.task(bind=True)
def createDiskFromImage(
    self, username, location, vm_size, operating_system, apps=[], ID=-1
):
    hr = 400
    payload = None
    attempts = 0
    disk_name = None

    while hr == 400 and attempts < 10:
        sendInfo(ID, "Creating {} disk for {}".format(operating_system, username))
        payload = createDiskFromImageHelper(
            username, location, vm_size, operating_system
        )
        hr = payload["status"]
        disk_name = payload["disk_name"]
        sendInfo(ID, "Disk {} created with status {}".format(disk_name, hr))
        attempts += 1

    if hr == 200 and disk_name and len(apps) > 0:
        sendInfo(ID, "Disk created, inserting apps {} for {}".format(apps, disk_name))
        insertDiskApps(disk_name, apps)

    sendDebug(ID, payload)
    payload["location"] = location
    return payload


@celery.task(bind=True)
def attachDisk(self, disk_name, vm_name, ID=-1):
    _, compute_client, _ = createClients()
    data_disk = compute_client.disks.get(os.environ.get("VM_GROUP"), disk_name)
    if data_disk.managed_by:
        old_vm_name = data_disk.managed_by.split("/")[-1]
        if old_vm_name != vm_name:
            print(
                "Disk {} already attached to {}. Detaching.".format(
                    disk_name, old_vm_name
                )
            )
            detachDisk(disk_name, old_vm_name)
        else:
            return {"status": 200, "disk_name": disk_name, "vm_name": vm_name}

    lunNum = 1
    attachedDisk = False
    while not attachedDisk and lunNum < 15:
        print("Trying to attach {} to {}".format(disk_name, vm_name))
        try:
            # Get the virtual machine by name
            print("Incrementing lun")
            virtual_machine = compute_client.virtual_machines.get(
                os.environ.get("VM_GROUP"), vm_name
            )
            print("Retrieved VM")
            virtual_machine.storage_profile.data_disks.append(
                {
                    "lun": lunNum,
                    "name": data_disk.name,
                    "create_option": DiskCreateOption.attach,
                    "managed_disk": {"id": data_disk.id},
                }
            )
            print("Appended data disk")

            async_disk_attach = compute_client.virtual_machines.create_or_update(
                os.environ.get("VM_GROUP"), virtual_machine.name, virtual_machine
            )
            async_disk_attach.wait()

            print("Done appending data disk")

            attachedDisk = True
        except ClientException as e:
            print(str(e))
            lunNum += 1

    if lunNum >= 15:
        return {"status": 404}

    async_disk_attach.wait()

    command = """
        Get-Disk | Where partitionstyle -eq 'raw' |
            Initialize-Disk -PartitionStyle MBR -PassThru |
            New-Partition -AssignDriveLetter -UseMaximumSize |
            Format-Volume -FileSystem NTFS -NewFileSystemLabel "{disk_name}" -Confirm:$false
        """.format(
        disk_name=disk_name
    )

    run_command_parameters = {"command_id": "RunPowerShellScript", "script": [command]}
    poller = compute_client.virtual_machines.run_command(
        os.environ.get("VM_GROUP"), vm_name, run_command_parameters
    )

    result = poller.result()
    print("Disk attached to LUN#" + str(lunNum))
    print(result.value[0].message)
    return {"status": 200, "disk_name": disk_name, "vm_name": vm_name}


@celery.task(bind=True)
def detachDisk(self, disk_name, vm_name, ID=-1):
    """Detaches disk from vm

    Args:
        vm_name (str): Name of the vm
        disk_name (str): Name of the disk
        ID (int, optional): Papertrail logging ID. Defaults to -1.
    """
    _, compute_client, _ = createClients()
    # Detach data disk
    sendInfo(ID, "\nDetach Data Disk: " + disk_name)

    virtual_machine = compute_client.virtual_machines.get(
        os.environ.get("VM_GROUP"), vm_name
    )
    data_disks = virtual_machine.storage_profile.data_disks
    data_disks[:] = [disk for disk in data_disks if disk.name != disk_name]
    async_vm_update = compute_client.virtual_machines.create_or_update(
        os.environ.get("VM_GROUP"), vm_name, virtual_machine
    )
    virtual_machine = async_vm_update.result()
    sendInfo(ID, "Detach data disk sucessful")
    return {"status": 200}


@celery.task(bind=True)
def fetchAll(self, update, ID=-1):
    _, compute_client, _ = createClients()
    vms = {"value": []}
    azure_portal_vms = compute_client.virtual_machines.list(os.getenv("VM_GROUP"))
    vm_usernames = []
    vm_names = []
    current_vms = {}
    vm_ip = None

    if update:
        current_vms = fetchUserVMs(None)

    for entry in azure_portal_vms:
        sendDebug(ID, "Found VM {} in Azure portal".format(entry.name))
        vm_info = {}

        try:
            if current_vms[entry.name]["ip"]:
                vm_ip = current_vms[entry.name]["ip"]
                vm_info = {
                    "vm_name": entry.name,
                    "username": current_vms[entry.name]["username"],
                    "ip": vm_ip,
                    "location": entry.location,
                }
            else:
                vm = getVM(entry.name)
                vm_ip = getIP(vm)
                vm_info = {
                    "vm_name": entry.name,
                    "username": current_vms[entry.name]["username"],
                    "ip": vm_ip,
                    "location": entry.location,
                }
                updateVMIP(entry.name, vm_ip)
        except:
            vm = getVM(entry.name)
            vm_ip = getIP(vm)
            vm_info = {
                "vm_name": entry.name,
                "username": entry.os_profile.admin_username,
                "ip": vm_ip,
                "location": entry.location,
            }

        vms["value"].append(vm_info)
        vm_usernames.append(entry.os_profile.admin_username)
        vm_names.append(entry.name)

        if update:
            sendDebug(ID, "Inserting {} into database".format(entry.name))
            insertVM(entry.name)

    if update:
        updateVMStates()
        if current_vms:
            for vm_name, vm_username in current_vms.items():
                deleteRow(vm_name, vm_names)

    return vms


@celery.task(bind=True)
def deleteVMResources(self, vm_name, delete_disk, ID=-1):
    if spinLock(vm_name) > 0:
        lockVMAndUpdate(
            vm_name=vm_name,
            state="DELETING",
            lock=True,
            temporary_lock=None,
            change_last_updated=True,
            verbose=False,
            ID=ID,
        )

        status = 200 if deleteResource(vm_name, delete_disk) else 404

        lockVM(vm_name, False)

        sendInfo(
            ID,
            "Disk vm deletion request for vm {} resolved with status {}".format(
                vm_name, status
            ),
        )

        return {"status": status}
    else:
        return {"status": 404}


@celery.task(bind=True)
def restartVM(self, vm_name, ID=-1):
    if spinLock(vm_name) > 0:
        lockVMAndUpdate(
            vm_name=vm_name,
            state="RESTARTING",
            lock=True,
            temporary_lock=None,
            change_last_updated=True,
            verbose=False,
            ID=ID,
        )

        _, compute_client, _ = createClients()

        fractalVMStart(vm_name, True)

        return {"status": 200}
    else:
        return {"status": 404}


@celery.task(bind=True)
def startVM(self, vm_name, ID=-1):
    sendInfo(ID, "Trying to start vm {}".format(vm_name))

    if spinLock(vm_name) > 0:
        lockVM(vm_name, True)

        _, compute_client, _ = createClients()

        fractalVMStart(vm_name)
        lockVM(vm_name, False)

        sendInfo(ID, "VM {} started successfully".format(vm_name))

        return {"status": 200}
    else:
        return {"status": 400}


@celery.task(bind=True)
def stopVM(self, vm_name, ID=-1):
    sendInfo(ID, "Trying to stop vm {}".format(vm_name))

    if spinLock(vm_name) > 0:
        lockVM(vm_name, True)

        _, compute_client, _ = createClients()

        stopVm(vm_name)
        lockVM(vm_name, False)

        sendInfo(ID, "VM {} stopped successfully".format(vm_name))

        return {"status": 200}
    else:
        return {"status": 400}


@celery.task(bind=True)
def updateVMStates(self, ID=-1):
    sendInfo(ID, "Updating vm states for all vms")
    _, compute_client, _ = createClients()
    vms = compute_client.virtual_machines.list(
        resource_group_name=os.environ.get("VM_GROUP")
    )

    # looping inside the list of virtual machines, to grab the state of each machine
    for vm in vms:
        state = ""
        is_running = False
        available = False
        vm_state = compute_client.virtual_machines.instance_view(
            resource_group_name=os.environ.get("VM_GROUP"), vm_name=vm.name
        )
        if "running" in vm_state.statuses[1].code:
            is_running = True

        username = fetchVMCredentials(vm.name)
        if username:
            username = username["username"]
            most_recent_action = getMostRecentActivity(username)
            if not most_recent_action:
                available = True
            elif most_recent_action["action"] == "logoff":
                available = True

        vm_info = compute_client.virtual_machines.get(
            os.environ.get("VM_GROUP"), vm.name
        )
        if len(vm_info.storage_profile.data_disks) == 0:
            available = True

        updateVMStateAutomatically(vm.name)

    return {"status": 200}


@celery.task(bind=True)
def syncDisks(self, ID=-1):
    """Syncs disks sql table to what's on Azure

    Returns:
        dict: status = 200 for success
    """
    sendInfo(ID, "Starting sync disks")
    _, compute_client, _ = createClients()
    disks = compute_client.disks.list(resource_group_name=os.environ.get("VM_GROUP"))
    disk_names = []

    for disk in disks:
        disk_name = disk.name
        disk_names.append(disk_name)
        disk_state = disk.disk_state
        vm_name = disk.managed_by
        if vm_name:
            vm_name = vm_name.split("/")[-1]
        else:
            vm_name = ""
        location = disk.location

        updateDisk(disk_name, vm_name, location)

    stored_disks = fetchUserDisks(None)
    for stored_disk in stored_disks:
        if not stored_disk["disk_name"] in disk_names:
            deleteDiskFromTable(stored_disk["disk_name"])
            stored_disks.remove(stored_disk)
        # else:
        #     insertDiskSetting(stored_disk["disk_name"], stored_disk["branch"], False)

    sendInfo(ID, "Sync disks complete")

    return {"status": 200}


@celery.task(bind=True)
def swapDiskSync(self, disk_name, ID=-1):
    def swapDiskAndUpdate(s, disk_name, vm_name, needs_winlogon):
        # Pick a VM, attach it to disk
        hr = swapdisk_name(s, disk_name, vm_name, needs_winlogon = needs_winlogon)
        if hr > 0:
            updateDisk(disk_name, vm_name, location)
            associateVMWithDisk(vm_name, disk_name)
            sendInfo(
                ID, "Database updated with VM {}, disk {}".format(vm_name, disk_name)
            )
            return 1
        else:
            return -1

    def updateOldDisk(vm_name):
        virtual_machine = getVM(vm_name)
        old_disk = virtual_machine.storage_profile.os_disk
        updateDisk(old_disk.name, "", None)

    def attachSecondaryDisks(s, username, vm_name):
        secondary_disks = fetchSecondaryDisks(username)
        if secondary_disks:
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

            s.update_state(
                state="PENDING",
                meta={"msg": "{} extra storage hard drive(s) were found on your cloud PC. Running a few extra tests.".format(len(secondary_disks))},
            )

            for secondary_disk in secondary_disks:
                attachDisk(secondary_disk["disk_name"], vm_name)

            # Lock immediately
            lockVMAndUpdate(
                vm_name=vm_name,
                state="RUNNING_AVAILABLE",
                lock=False,
                temporary_lock=1,
                change_last_updated=True,
                verbose=False,
                ID=ID,
            )

    sendInfo(ID, " Swap disk task for disk {} added to Redis queue".format(disk_name))

    # Get the
    _, compute_client, _ = createClients()

    os_disk = compute_client.disks.get(os.environ.get("VM_GROUP"), disk_name)
    os_type = 'Windows' if 'windows' in str(os_disk.os_type) else 'Linux'
    needs_winlogon = os_type == 'Windows'
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

                if fractalVMStart(vm_name, needs_winlogon = needs_winlogon, s=self) > 0:
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

                    vm_info = compute_client.virtual_machines.get(os.getenv('VM_GROUP'), vm_name)

                    for disk in vm_info.storage_profile.data_disks:
                        if disk.name != disk_name:
                            self.update_state(
                                state="PENDING",
                                meta={"msg": "Making sure that you have a stable connection."},
                            )

                            sendInfo(
                                ID,
                                "Detaching disk {} from {}".format(
                                    disk_name, vm_name
                                ),
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

    def swapDiskAndUpdate(s, disk_name, vm_name, needs_winlogon):
        # Pick a VM, attach it to disk
        sendInfo(ID, "Preparing to swap disk")
        hr = swapdisk_name(s, disk_name, vm_name, needs_winlogon=needs_winlogon)
        if hr > 0:
            updateDisk(disk_name, vm_name, location)
            associateVMWithDisk(vm_name, disk_name)
            sendInfo(
                ID, "Database updated with VM {}, disk {}".format(vm_name, disk_name)
            )
            return 1
        else:
            return -1

    def updateOldDisk(vm_name):
        virtual_machine = getVM(vm_name)
        old_disk = virtual_machine.storage_profile.os_disk
        updateDisk(old_disk.name, "", None)

    def attachSecondaryDisks(s, username, vm_name):
        secondary_disks = fetchSecondaryDisks(username)
        if secondary_disks:
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

            s.update_state(
                state="PENDING",
                meta={
                    "msg": "{} extra storage hard drive(s) were found on your cloud PC. Running a few extra tests.".format(
                        len(secondary_disks)
                    )
                },
            )

            for secondary_disk in secondary_disks:
                attachDisk(secondary_disk["disk_name"], vm_name)

            # Lock immediately
            lockVMAndUpdate(
                vm_name=vm_name,
                state="RUNNING_AVAILABLE",
                lock=False,
                temporary_lock=1,
                change_last_updated=True,
                verbose=False,
                ID=ID,
            )

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


@celery.task(bind=True)
def swapSpecificDisk(self, disk_name, vm_name, ID=-1):
    """Swaps out a disk in a vm

    Args:
        disk_name (str): The name of the disk to swap out
        vm_name (str): The name of the vm the disk is attached to
        ID (int, optional): Papertrail logging ID. Defaults to -1.

    Returns:
        dict: A dictionary representing the VM in the v_ms sql table
    """
    sendInfo(ID, "Attempting to swap out disk {} in VM {}".format(disk_name, vm_name))
    locked = checkLock(vm_name)

    while locked:
        sendDebug(ID, "VM {} is locked. Waiting to be unlocked".format(vm_name))
        time.sleep(5)
        locked = checkLock(vm_name)

    lockVM(vm_name, True, ID=ID)

    _, compute_client, _ = createClients()
    new_os_disk = compute_client.disks.get(os.environ.get("VM_GROUP"), disk_name)

    vm = getVM(vm_name)
    vm.storage_profile.os_disk.managed_disk.id = new_os_disk.id
    vm.storage_profile.os_disk.name = new_os_disk.name

    sendDebug(ID, "Swapping out disk " + disk_name + " on VM " + vm_name)
    start = time.perf_counter()

    async_disk_attach = compute_client.virtual_machines.create_or_update(
        "Fractal", vm.name, vm
    )
    async_disk_attach.wait()

    end = time.perf_counter()
    # sendDebug(ID, f"SUCCESS: Disk swapped out in {end - start:0.4f} seconds. Restarting " + vm_name)

    start = time.perf_counter()
    fractalVMStart(vm_name)
    end = time.perf_counter()

    # sendDebug(ID, f"SUCCESS: VM restarted in {end - start:0.4f} seconds")

    updateDisk(disk_name, vm_name, None)
    associateVMWithDisk(vm_name, disk_name)
    sendDebug(ID, "Database updated.")

    time.sleep(10)

    sendDebug(ID, "VM " + vm_name + " successfully restarted")

    lockVM(vm_name, False, ID=ID)
    return fetchVMCredentials(vm_name)


@celery.task(bind=True)
def updateVMTable(self, ID=-1):
    """Updates the entire VM sql table

    Args:
        ID (int, optional): Papertrail logging id. Defaults to -1.

    Returns:
        dict: status 200 for success
    """
    sendInfo(ID, "Updating VM table")
    vms = fetchUserVMs(None, ID)
    _, compute_client, network_client = createClients()
    azure_portal_vms = [
        entry.name
        for entry in compute_client.virtual_machines.list(os.getenv("VM_GROUP"))
    ]

    for vm in vms:
        try:
            if not vm["vm_name"] in azure_portal_vms:
                deleteVMFromTable(vm["vm_name"])
            else:
                vm_name = vm["vm_name"]
                vm = getVM(vm_name)
                os_disk_name = vm.storage_profile.os_disk.name
                username = mapDiskToUser(os_disk_name)
                updateVM(vm_name, vm.location, os_disk_name, username)
        except Exception as e:
            sendError(ID, e)
            pass

    return {"status": 200}


@celery.task(bind=True)
def runPowershell(self, vm_name, ID=-1):
    _, compute_client, _ = createClients()
    with open("app/scripts/vmCreate.txt", "r") as file:
        sendInfo(ID, "TASK: Starting to run Powershell scripts")
        command = file.read()
        run_command_parameters = {
            "command_id": "RunPowerShellScript",
            "script": [command],
        }

        poller = compute_client.virtual_machines.run_command(
            os.environ.get("VM_GROUP"), vm_name, run_command_parameters
        )
        # poller.wait()
        result = poller.result()
        sendInfo(ID, "SUCCESS: Powershell scripts finished running")
        sendDebug(ID, result.value[0].message)

    return {"status": 200}

@celery.task(bind=True)
def deleteDisk(self, disk_name, ID=-1):
    _, compute_client, _ = createClients()
    try:
        sendInfo(ID, "Attempting to delete the OS disk...")
        os_disk_delete = compute_client.disks.delete(os.getenv("VM_GROUP"), disk_name)
        os_disk_delete.wait()
        deleteDiskSettingsFromTable(disk_name)
        deleteDiskFromTable(disk_name)
        sendInfo(ID, "OS disk deleted")
    except Exception as e:
        sendError(ID, "ERROR: " + str(e))
        updateDiskState(disk_name, "TO_BE_DELETED")

    sendInfo(ID, "SUCCESS: {} deleted".format(disk_name))

    return {"status": 200}


@celery.task(bind=True)
def deallocateVM(self, vm_name, ID=-1):
    lockVM(vm_name, True, ID=ID)

    _, compute_client, _ = createClients()

    sendInfo(ID, "Starting to deallocate VM {}".format(vm_name))
    updateVMState(vm_name, "DEALLOCATING")

    async_vm_deallocate = compute_client.virtual_machines.deallocate(
        os.environ.get("VM_GROUP"), vm_name
    )
    async_vm_deallocate.wait()

    lockVM(vm_name, False, ID=ID)

    updateVMState(vm_name, "DEALLOCATED")
    sendInfo(ID, "VM {} deallocated successfully".format(vm_name))

    return {"status": 200}


@celery.task(bind=True)
def storeLogs(self, sender, connection_id, logs, vm_ip, version, ID=-1):
    if SendLogsToS3(logs, sender, connection_id, vm_ip, version, ID) > 0:
        return {"status": 200}
    return {"status": 422}


@celery.task(bind=True)
def deleteLogs(self, connection_id, ID=-1):
    if deleteLogsInS3(connection_id, ID) > 0:
        sendInfo(ID, "Delete logs success")
        return {"status": 200}
    sendError(ID, "Delete logs unsuccessful")
    return {"status": 422}
