from app import *
from app.helpers.utils.azure.azure_general import *
from app.helpers.utils.azure.azure_resource_locks import *
from app.helpers.utils.azure.azure_helpers.azure_resource_state_management_helpers import *


def sendVMStartCommand(
    vm_name, needs_restart, needs_winlogon, resource_group=os.getenv("VM_GROUP"), s=None
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

        output = fractalSQLSelect(
            table_name=resourceGroupToTable(resource_group), params={"vm_name": vm_name}
        )

        disk_name = None
        username = None
        if output["success"] and output["rows"]:
            disk_name = output["rows"][0]["disk_name"]
            username = output["rows"][0]["username"]
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
                winlogon = waitForWinlogon(vm_name, resource_group)

                # If we did not receive a winlogon ping, reboot the VM and keep waiting

                number_of_winlogon_tries = 1

                while winlogon < 0:
                    if number_of_winlogon_tries > 4:
                        fractalLog(
                            function="sendVMStartCommand",
                            label=str(username),
                            logs="Could not detect a winlogon on VM {vm_name} with disk {disk_name} after 4 tries".format(
                                vm_name=vm_name, disk_name=disk_name
                            ),
                            level=logging.CRITICAL,
                        )
                        return -1

                    boot_if_necessary(vm_name, True, resource_group=resource_group, s=s)
                    winlogon = waitForWinlogon(vm_name, resource_group, s=s)

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


def fractalVMStart(
    vm_name,
    needs_restart=False,
    needs_winlogon=True,
    resource_group=os.getenv("VM_GROUP"),
    s=None,
):
    """Bullies Azure into actually starting the vm by repeatedly calling sendVMStartCommand if necessary 
    (big brain thoughts from Ming)

    Args:
        vm_name (str): Name of the vm to start
        needs_restart (bool, optional): Whether the vm needs to restart after. Defaults to False.

    Returns:
        int: 1 for success, -1 for failure
    """

    fractalLog(
        function="fractalVMStart",
        label=getVMUser(vm_name, resource_group),
        logs="Starting VM {vm_name} in resource group {resource_group}, need_restart is {needs_restart}, needs_winlogon is {needs_winlogon}".format(
            vm_name=vm_name,
            resource_group=resource_group,
            needs_restart=str(needs_restart),
            needs_winlogon=str(needs_winlogon),
        ),
    )

    _, compute_client, _ = createClients()

    started = False
    start_attempts = 0

    # We will try to start/restart the VM and wait for it three times in total before giving up
    while not started and start_attempts < 3:
        start_command_tries = 0

        # First, send a basic start or restart command. Try three times, if it fails, give up

        if s:
            s.update_state(
                state="PENDING",
                meta={"msg": "Cloud PC successfully received boot request."},
            )

        while (
            sendVMStartCommand(
                vm_name, needs_restart, needs_winlogon, resource_group, s=s
            )
            < 0
            and start_command_tries < 4
        ):
            time.sleep(10)
            start_command_tries += 1

        if start_command_tries >= 4:
            return -1

        wake_retries = 0

        # After the VM has been started/restarted, query the state. Try 12 times for the state to be running. If it is not running,
        # give up and go to the top of the while loop to send another start/restart command

        vm_state = compute_client.virtual_machines.instance_view(
            resource_group_name=resource_group, vm_name=vm_name
        )

        # Success! VM is running and ready to use

        if "running" in vm_state.statuses[1].code:
            lockVMAndUpdate(
                vm_name, "RUNNING_AVAILABLE", False, temporary_lock=None,
            )
            started = True
            return 1

        # Wait a minute to see if the state changes to running

        while not "running" in vm_state.statuses[1].code and wake_retries < 12:
            fractalLog(
                function="fractalVMStart",
                label=getVMUser(vm_name, resource_group),
                logs="After sending a start command, VM {vm_name} is still in state {state}".format(
                    vm_name=vm_name, state=vm_state.statuses[1].code
                ),
                level=logging.WARNING,
            )

            time.sleep(5)

            vm_state = compute_client.virtual_machines.instance_view(
                resource_group_name=resource_group, vm_name=vm_name
            )

            # Success! VM is running and ready to use

            if "running" in vm_state.statuses[1].code:
                lockVMAndUpdate(
                    vm_name, "RUNNING_AVAILABLE", False, temporary_lock=None,
                )
                started = True
                return 1

            wake_retries += 1

        start_attempts += 1

    return -1
