import boto3
import botocore
import paramiko

def sendVMStartCommand(
    vm_name, needs_restart, needs_winlogon, resource_group=VM_GROUP, s=None
):
    """Starts a VM

    Args:
        vm_name (str): The name of the vm to start
        needs_restart (bool): Whether the VM needs to restart after
        needs_winlogon (bool): Whether the VM is a Windows machine

    Returns:
        int: 1 for success, -1 for fail
    """
    try:
        lockVMAndUpdate(vm_name, "ATTACHING", True, temporary_lock=4)

        if s:
            s.update_state(
                state="PENDING",
                meta={"msg": "Cloud PC started executing boot request."},
            )

        # Fetch the name of the disk currently attached to the VM

        vm = UserVM.query.get(vm_name)

        disk_name = None
        username = None
        if vm:
            disk_name = vm.disk_id
            username = vm.user_id
        else:
            fractalLog(
                function="sendVMStartCommand",
                label=vm_name,
                logs="Could not find a VM named {vm_name} in table {table_name}".format(
                    vm_name=vm_name,
                    table_name=str(resourceGroupToTable(resource_group)),
                ),
                level=logging.ERROR,
            )
            return -1

        # Check if the disk has been initially set up

        num_boots = 1 if not checkFirstTime(disk_name) else 2

        # Run app setup scripts if the disk has not been initially set up

        for i in range(0, num_boots):
            if i == 1:
                needs_restart = True

                if s:
                    s.update_state(
                        state="PENDING",
                        meta={
                            "msg": "Since this is your first time logging on, we're running a few extra tests. Please allow a few extra minutes."
                        },
                    )

                time.sleep(10)

            if s:
                s.update_state(
                    state="PENDING",
                    meta={"msg": "Cloud PC still executing boot request."},
                )

            # Power Azure VM on if it's not currently running

            boot_if_necessary(
                vm_name, needs_restart, resource_group=resource_group, s=s
            )

            # If a Windows VM, wait for a winlogon ping, which indicates that the server-side protocol is running

            if needs_winlogon:
                winlogon = waitForWinlogon(vm_name, resource_group, s=s)

                # If we did not receive a winlogon ping, reboot the VM and keep waiting

                number_of_winlogon_tries = 1

                while not winlogon:
                    if number_of_winlogon_tries > 4:
                        fractalLog(
                            function="sendVMStartCommand",
                            label=str(username),
                            logs="Could not detect a winlogon on VM {vm_name} with disk {disk_name} after 4 tries".format(
                                vm_name=vm_name, disk_name=disk_name
                            ),
                            level=logging.CRITICAL,
                        )

                        if s:
                            s.update_state(
                                state="PENDING",
                                meta={
                                    "msg": "Could not log you in after 4 attempts. Trying 4 more times before forcefully quitting..."
                                },
                            )
                            return -1

                    boot_if_necessary(vm_name, True, resource_group=resource_group, s=s)
                    winlogon = waitForWinlogon(vm_name, resource_group, s=s)
                    number_of_winlogon_tries += 1

                if s:
                    s.update_state(
                        state="PENDING",
                        meta={"msg": "Logged into your cloud PC successfully."},
                    )

            # Finally, run app installation scripts if the disk has not been set up yet

            if i == 1:
                changeFirstTime(disk_name)

                if s:
                    s.update_state(
                        state="PENDING",
                        meta={
                            "msg": "Pre-installing your applications now. This could take several minutes."
                        },
                    )

                apps = AppsToInstall.query.filter_by(disk_id=disk_name).all()

                if apps:
                    if installApplications(vm_name, apps, vm.os) < 0:
                        fractalLog(
                            function="sendVMStartCommand",
                            label=str(username),
                            logs="VM {vm_name} errored out while installing applications".format(
                                vm_name=vm_name
                            ),
                            level=logging.ERROR,
                        )

            # VM is receive connections, remove permanent lock

            lockVMAndUpdate(
                vm_name, "RUNNING_AVAILABLE", False, temporary_lock=2,
            )
        return 1
    except Exception as e:
        fractalLog(
            function="sendVMStartCommand",
            label=str(username),
            logs="Encountered a critical error while starting VM {vm_name}: {error}".format(
                vm_name=vm_name, error=str(e)
            ),
            level=logging.CRITICAL,
        )
        fractalLog(
            function="sendVMStartCommand",
            label=str(username),
            logs="Traceback: {traceback}".format(traceback=str(traceback.format_exc())),
            level=logging.CRITICAL,
        )
        return -1