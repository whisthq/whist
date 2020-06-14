from app import *
from app.helpers.utils.azure.azure_general import *
from app.helpers.utils.azure.azure_helpers.azure_resource_state_management_helpers import *


def sendVMStartCommand(vm_name, needs_restart, needs_winlogon, s=None):
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

        output = fractalSQLSelect(table_name="v_ms", params={"vm_name": vm_name})

        disk_name = None
        if output["success"] and output["rows"]:
            disk_name = output["rows"][0]["disk_name"]
        else:
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

            boot_if_necessary(vm_name, needs_restart, s=s)

            # If a Windows VM, wait for a winlogon ping, which indicates that the server-side protocol is running

            if needs_winlogon:
                winlogon = waitForWinlogon(vm_name)

                # If we did not receive a winlogon ping, reboot the VM and keep waiting

                number_of_winlogon_tries = 1

                while winlogon < 0:
                    if number_of_winlogon_tries > 4:
                        fractalLog(
                            "sendVMStartCommand() could not detect a winlogon on VM {vm_name} with disk {disk_name} after 4 tries".format(
                                vm_name=vm_name, disk_name=disk_name
                            ),
                            level=logging.CRITICAL,
                        )
                        return -1

                    boot_if_necessary(vm_name, True)
                    winlogon = waitForWinlogon(vm_name)

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

                output = fractalSQLSelect(
                    table_name="disk_apps", params={"disk_name": disk_name}
                )

                if output["success"] and output["rows"]:
                    if installApplications(vm_name, output["rows"]) < 0:
                        fractalLog(
                            "VM {vm_name} errored out while installing applications".format(
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
            "sendVMStartCommand() encountered a critical error while starting VM {vm_name}: {error}".format(
                vm_name=vm_name, error=str(e)
            ),
            level=logging.CRITICAL,
        )
        return -1
