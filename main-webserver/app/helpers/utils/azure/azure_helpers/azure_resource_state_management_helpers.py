from app import *
from app.helpers.utils.azure.azure_general import *
from app.helpers.utils.azure.azure_resource_locks import *


def boot_if_necessary(
    vm_name, needs_restart, resource_group=os.getenv("VM_GROUP"), s=None
):
    _, compute_client, _ = createClients()

    power_state = "PowerState/deallocated"
    vm_state = compute_client.virtual_machines.instance_view(
        resource_group_name=resource_group, vm_name=vm_name
    )

    try:
        power_state = vm_state.statuses[1].code
    except Exception as e:
        fractalLog(
            function="sendVMStartCommand",
            label="VM {vm_name}".format(vm_name=vm_name),
            logs="Not able to retrieve state for {vm_name}".format(vm_name),
            level=logging.ERROR,
        )
        pass

    if "stop" in power_state or "dealloc" in power_state:
        if s:
            s.update_state(
                state="PENDING",
                meta={
                    "msg": "Your cloud PC is powered off. Powering on (this could take a few minutes)."
                },
            )

        fractalLog(
            function="sendVMStartCommand",
            label=getVMUser(vm_name, resource_group),
            logs="Detected that VM {vm_name} is in state {power_state}. starting to boot.".format(
                vm_name=vm_name, power_state=power_state
            ),
        )

        lockVMAndUpdate(vm_name, "STARTING", True, temporary_lock=10)

        async_vm_start = compute_client.virtual_machines.start(resource_group, vm_name)

        async_vm_start.wait()

        fractalLog(
            function="sendVMStartCommand",
            label=getVMUser(vm_name, resource_group),
            logs="VM {vm_name} started successfully.".format(vm_name=vm_name),
        )

        if s:
            s.update_state(
                state="PENDING",
                meta={"msg": "Your cloud PC was started successfully."},
            )

        return 1

    if needs_restart:
        if s:
            s.update_state(
                state="PENDING",
                meta={
                    "msg": "Your cloud PC needs to be restarted. Restarting (this will take no more than a minute)."
                },
            )

        fractalLog(
            function="sendVMStartCommand",
            label="VM {vm_name}".format(vm_name=vm_name),
            logs="Detected that VM {vm_name} needs to be restarted".format(
                vm_name=vm_name
            ),
        )

        lockVMAndUpdate(vm_name, "RESTARTING", True, temporary_lock=10)

        async_vm_restart = compute_client.virtual_machines.restart(
            resource_group, vm_name
        )

        async_vm_restart.wait()

        if s:
            s.update_state(
                state="PENDING",
                meta={"msg": "Your cloud PC was restarted successfully."},
            )

        return 1
    return 1


def checkFirstTime(disk_name):
    output = fractalSQLSelect("disks", params={"disk_name": disk_name})

    if output["success"] and output["rows"]:
        return output["rows"][0]["first_time"]

    return False


def changeFirstTime(disk_name, first_time=False):
    fractalSQLUpdate(
        table_name="disks",
        conditional_params={"disk_name": disk_name},
        new_params={"first_time": first_time},
    )


def waitForWinlogon(vm_name, resource_group=os.getenv("VM_GROUP"), s=None):
    """Periodically checks and sleeps until winlogon succeeds

    Args:
        vm_name (str): Name of the vm

    Returns:
        int: 1 for success, -1 for fail
    """

    # Check if a VM has winlogon'ed within the last 10 seconds

    def checkWinlogon(vm_name, resource_group):
        output = fractalSQLSelect(
            table_name=resourceGroupToTable(resource_group), params={"vm_name": vm_name}
        )

        has_winlogoned = False
        last_winlogon_timestamp = 0

        if output["success"] and output["rows"]:
            if not output["rows"][0]["ready_to_connect"]:
                return False
            else:
                has_winlogoned = (
                    dateToUnix(getToday()) - output["rows"][0]["ready_to_connect"] < 8
                )
                last_winlogon_timestamp = output["rows"][0]["ready_to_connect"]
        else:
            fractalLog(
                function="waitForWinlogon",
                label=getVMUser(vm_name),
                logs="Could not find VM {vm_name} in resource group {resource_group}".format(
                    vm_name=vm_name, resource_group=resource_group
                ),
                level=logging.ERROR,
            )

            return False

        if has_winlogoned:
            fractalLog(
                function="waitForWinlogon",
                label=getVMUser(vm_name),
                logs="Detected initial winlogon event within last 8 seconds on VM {vm_name}. Checking for a second winlogon event...".format(
                    vm_name=vm_name
                ),
            )

            time.sleep(4)

            num_queries = 0

            while num_queries < 5:
                output = fractalSQLSelect(
                    table_name=resourceGroupToTable(resource_group),
                    params={"vm_name": vm_name},
                )
                if output["success"] and output["rows"]:
                    if not output["rows"][0]["ready_to_connect"]:
                        return False
                    if output["rows"][0]["ready_to_connect"] > last_winlogon_timestamp:
                        fractalLog(
                            function="waitForWinlogon",
                            label=getVMUser(vm_name),
                            logs="VM {vm_name} had another winlogon event within the last 4 seconds. VM winlogon success!".format(
                                vm_name=vm_name
                            ),
                        )
                        return True
                    else:
                        time.sleep(1)
                        num_queries += 1
                else:
                    fractalLog(
                        function="waitForWinlogon",
                        label=getVMUser(vm_name),
                        logs="Could not find VM {vm_name} in resource group {resource_group} on second winlogon query".format(
                            vm_name=vm_name, resource_group=resource_group
                        ),
                        level=logging.ERROR,
                    )

                    return False

            fractalLog(
                function="waitForWinlogon",
                label=getVMUser(vm_name),
                logs="VM {vm_name} had a winlogon event 8 seconds ago, but after waiting another 9 seconds, another winlogon event was not detected. Assuming that the protocol died sometime in the last few seconds and returning winlogon False".format(
                    vm_name=vm_name
                ),
                level=logging.WARNING,
            )
            return False

        return False

    # Check if a VM is a dev machine

    def checkDev(vm_name, resource_group):
        output = fractalSQLSelect(
            table_name=resourceGroupToTable(resource_group), params={"vm_name": vm_name}
        )

        if output["success"] and output["rows"]:
            return output["rows"][0]["dev"]

        return False

    if s:
        s.update_state(
            state="PENDING",
            meta={
                "msg": "Logging you into your cloud PC. This should take less than two minutes."
            },
        )

<<<<<<< HEAD
    has_winlogoned = checkWinlogon(vm_name, resource_group)
    num_tries = 0

    # Return success if a winlogon has been detected within the last 10 seconds

    if (
        has_winlogoned
        or checkDev(vm_name, resource_group)
        or resource_group != os.getenv("VM_GROUP")
    ):
=======
    fractalLog(
        function="waitForWinlogon",
        label=getVMUser(vm_name),
        logs="Checking to see if VM {vm_name} has a recent winlogon event".format(
            vm_name=vm_name
        ),
    )

    has_winlogoned = checkWinlogon(vm_name, resource_group)
    num_tries = 0

    if has_winlogoned:
        fractalLog(
            function="waitForWinlogon",
            label=getVMUser(vm_name),
            logs="VM {vm_name} winlogoned on the first try!".format(vm_name=vm_name),
        )
    else:
        fractalLog(
            function="waitForWinlogon",
            label=getVMUser(vm_name),
            logs="VM {vm_name} has not winlogoned yet. Waiting...".format(
                vm_name=vm_name
            ),
        )

    # Return success if a winlogon has been detected within the last 10 seconds

    if checkDev(vm_name, resource_group):
        fractalLog(
            function="waitForWinlogon",
            label=getVMUser(vm_name),
            logs="VM {vm_name} is in dev mode. Winlogon will not be detected if protocol is not running.".format(
                vm_name=vm_name
            ),
        )

    if has_winlogoned or resource_group != os.getenv("VM_GROUP"):
        if resource_group != os.getenv("VM_GROUP"):
            fractalLog(
                function="waitForWinlogon",
                label=getVMUser(vm_name),
                logs="Resource group {resource_group} is not production resource group. Bypassing winlogon...".format(
                    resource_group=resource_group
                ),
            )
>>>>>>> staging
        return 1
    else:
        fractalLog(
            function="sendVMStartCommand",
            label="VM {vm_name}".format(vm_name=vm_name),
            logs="Detected that VM {vm_name} has not winlogon'ed. Waiting...".format(
                vm_name=vm_name
            ),
        )

    # Loop 30 times at 5 second intervals, waiting for a winlogon to come in

    while not has_winlogoned:
        time.sleep(5)
        has_winlogoned = checkWinlogon(vm_name, resource_group)
        num_tries += 1

        # Give up if we've waited too long

        if num_tries > 25:
            fractalLog(
                function="sendVMStartCommand",
                label="VM {vm_name}".format(vm_name=vm_name),
                logs="VM {vm_name} did not winlogon after 25 tries. Giving up...".format(
                    vm_name=vm_name
                ),
                level=logging.WARNING,
            )
            return False

    fractalLog(
        function="sendVMStartCommand",
        label="VM {vm_name}".format(vm_name=vm_name),
        logs="VM {vm_name} winlogoned successfully after {num_tries} tries".format(
            vm_name=vm_name, num_tries=str(num_tries)
        ),
    )
    return True


def installApplications(vm_name, apps):
    _, compute_client, _ = createClients()
    try:
        for app in apps:
            fractalLog(
                function="installApplications",
                label="VM {vm_name}".format(vm_name=vm_name),
                logs="Installing {app} onto VM {vm_name}".format(
                    app=app["app_name"], vm_name=vm_name
                ),
            )
            install_command = fetchInstallCommand(app["app_name"])

            run_command_parameters = {
                "command_id": "RunPowerShellScript",
                "script": [install_command["command"]],
            }

            poller = compute_client.virtual_machines.run_command(
                os.environ.get("VM_GROUP"), vm_name, run_command_parameters
            )

            result = poller.result()

            fractalLog(
                function="installApplications",
                label="VM {vm_name}".format(vm_name=vm_name),
                logs="App installation finished with message: {result}".format(
                    result=str(result.value[0].message)
                ),
            )

    except Exception as e:
        fractalLog(
            function="installApplications",
            label="VM {vm_name}".format(vm_name=vm_name),
            logs="Encountered an error while installing apps onto VM {vm_name}: {error}".format(
                vm_name=vm_name, error=str(e)
            ),
        )
        return -1
    return 1
