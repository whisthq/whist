from app import *
from app.helpers.utils.azure.azure_general import *
from app.helpers.utils.azure.azure_resource_locks import *
from app.helpers.utils.azure.azure_resource_state_management import *
from app.helpers.utils.azure.azure_resource_modification import *

from app.models.public import *
from app.serializers.public import *

user_vm_schema = UserVMSchema()


def attachSecondaryDisk(disk_name, vm_name, resource_group, s=None):
    fractalLog(
        function="attachSecondaryDisk",
        label=getVMUser(vm_name),
        logs="Attaching secondary disk {disk_name} to {vm_name}".format(
            disk_name=disk_name, vm_name=vm_name
        ),
    )

    _, compute_client, _ = createClients()
    data_disk = compute_client.disks.get(resource_group, disk_name)
    virtual_machine = createVMInstance(vm_name, resource_group)

    if data_disk.managed_by:
        old_vm_name = data_disk.managed_by.split("/")[-1]
        if old_vm_name != vm_name:
            detachSecondaryDisk(disk_name, old_vm_name, resource_group, s=s)
        else:
            return 1

    lunNum = 1
    attachedDisk = False
    while not attachedDisk and lunNum < 15:
        try:
            virtual_machine.storage_profile.data_disks.append(
                {
                    "lun": lunNum,
                    "name": data_disk.name,
                    "create_option": DiskCreateOption.attach,
                    "managed_disk": {"id": data_disk.id},
                }
            )

            async_disk_attach = compute_client.virtual_machines.create_or_update(
                resource_group, virtual_machine.name, virtual_machine
            )
            async_disk_attach.wait()

            attachedDisk = True

            fractalLog(
                function="attachSecondaryDisk",
                label=getVMUser(vm_name),
                logs="Secondary disk {disk_name} successfully attached to {vm_name}".format(
                    disk_name=disk_name, vm_name=vm_name
                ),
            )

        except ClientException as e:
            fractalLog(
                function="attachSecondaryDisk",
                label=getVMUser(vm_name),
                logs="Incrementing LUN, disk {disk_name} could not be attached to {vm_name} with error: {error}".format(
                    disk_name=disk_name, vm_name=vm_name, error=str(e)
                ),
                level=logging.WARNING,
            )
            lunNum += 1

    if lunNum >= 15:
        fractalLog(
            function="attachSecondaryDisk",
            label=getVMUser(vm_name),
            logs="Disk {disk_name} could not be attached to {vm_name} because there was not an available LUN".format(
                disk_name=disk_name, vm_name=vm_name
            ),
            level=logging.ERROR,
        )
        return -1

    fractalLog(
        function="attachSecondaryDisk",
        label=getVMUser(vm_name),
        logs="Running disk partition powershell script on {disk_name}".format(disk_name=disk_name),
    )

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
        resource_group, vm_name, run_command_parameters
    )

    result = poller.result()

    fractalLog(
        function="attachSecondaryDisk",
        label=getVMUser(vm_name),
        logs="Disk partition powershell script successfully run on {disk_name}".format(
            disk_name=disk_name
        ),
    )

    return 1


def attachSecondaryDisks(username, vm_name, resource_group, s=None):
    fractalLog(
        function="attachSecondaryDisks",
        label=getVMUser(vm_name),
        logs="Looking for any secondary disks associated with VM {vm_name}".format(vm_name=vm_name),
    )

    secondary_disks = SecondaryDisk.query.filter_by(user_id=username).all()

    if secondary_disks:
        fractalLog(
            function="attachSecondaryDisks",
            label=getVMUser(vm_name),
            logs="Found {num_disks} secondary disks associated with VM {vm_name}".format(
                num_disks=str(len(secondary_disks)), vm_name=vm_name
            ),
        )

        lockVMAndUpdate(
            vm_name=vm_name,
            state="ATTACHING",
            lock=True,
            temporary_lock=None,
            resource_group=resource_group,
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
            attachSecondaryDisk(secondary_disk.disk_id, vm_name, resource_group=resource_group, s=s)

        # Lock immediately
        lockVMAndUpdate(
            vm_name=vm_name,
            state="RUNNING_AVAILABLE",
            lock=False,
            temporary_lock=1,
            resource_group=resource_group,
        )
    else:
        fractalLog(
            function="attachSecondaryDisks",
            label=getVMUser(vm_name),
            logs="Did not find any secondary disks associated with VM {vm_name}".format(
                vm_name=vm_name
            ),
        )


def claimAvailableVM(username, disk_name, location, resource_group, os_type="Windows", s=None):
    # available_vm = UserVM.query.filter_by(user_id=username).first()

    # if available_vm:
    #     fractalLog(function="", label="", logs=str(available_vm))
    #     return available_vm

    state_preference = ["RUNNING_AVAILABLE", "STOPPED", "DEALLOCATED"]

    for state in state_preference:
        fractalLog(
            function="claimAvailableVM",
            label=str(username),
            logs="Querying all {location} {operating_system} VMs with state {state}".format(
                state=state, location=location, operating_system=os_type
            ),
        )

        available_vm = (
            UserVM.query.filter(
                UserVM.lock == False,
                UserVM.state == state,
                UserVM.os == os_type,
                UserVM.location == location,
            )
            .filter(
                (UserVM.temporary_lock <= dateToUnix(getToday())) | (UserVM.temporary_lock == None)
            )
            .first()
        )

        fractalLog(function="claimAvailableVM", label=str(username), logs=str(available_vm))

        if available_vm:
            fractalLog(
                function="claimAvailableVM",
                label=str(username),
                logs="Found a {location} VM with state {state} to attach {disk_name} to: VM {vm_name}".format(
                    location=location, state=state, disk_name=disk_name, vm_name=available_vm.vm_id
                ),
            )

            if s:
                if state == "RUNNING_AVAILABLE":
                    s.update_state(
                        state="PENDING",
                        meta={
                            "msg": "Your cloud PC is powered on. Preparing your cloud PC (this could take a few minutes)."
                        },
                    )
                else:
                    s.update_state(
                        state="PENDING",
                        meta={
                            "msg": "Your cloud PC is powered off. Preparing your cloud PC (this could take a few minutes)."
                        },
                    )

            new_params = {
                "lock": True,
                "user_id": username,
                "disk_id": disk_name,
                "state": "ATTACHING",
            }

            fractalSQLCommit(db, fractalSQLUpdate, available_vm, new_params)

            available_vm = user_vm_schema.dump(available_vm)

            return available_vm
        else:
            fractalLog(
                function="claimAvailableVM",
                label=str(username),
                logs="Did not find any VMs in {location} with state {state}.".format(
                    location=location, state=state
                ),
                level=logging.WARNING,
            )

    return None
