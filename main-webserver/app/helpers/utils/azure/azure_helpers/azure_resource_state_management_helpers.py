from app import *
from app.helpers.utils.azure.azure_general import *
from app.helpers.utils.azure.azure_resource_locks import *


def boot_if_necessary(vm_name, needs_restart, s=None):
    _, compute_client, _ = createClients()

    power_state = "PowerState/deallocated"
    vm_state = compute_client.virtual_machines.instance_view(
        resource_group_name=os.getenv("VM_GROUP"), vm_name=vm_name
    )

    try:
        power_state = vm_state.statuses[1].code
    except Exception as e:
        fractalLog(
            "sendVMStartcommand() was not able to retrieve state for {vm_name}".format(
                vm_name
            ),
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
            "sendVMStartCommand() detected that VM {vm_name} is in state {power_state}. starting to boot.".format(
                vm_name=vm_name, power_state=power_state
            )
        )

        lockVMAndUpdate(vm_name, "STARTING", True, temporary_lock=10)

        async_vm_start = compute_client.virtual_machines.start(
            os.environ.get("VM_GROUP"), vm_name
        )

        async_vm_start.wait()

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
            "sendVMStartCommand() detected that VM {vm_name} needs to be restarted".format(
                vm_name=vm_name
            )
        )

        lockVMAndUpdate(vm_name, "RESTARTING", True, temporary_lock=10)

        async_vm_restart = compute_client.virtual_machines.restart(
            os.environ.get("VM_GROUP"), vm_name
        )

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


def waitForWinlogon(vm_name, s=None):
    """Periodically checks and sleeps until winlogon succeeds

    Args:
        vm_name (str): Name of the vm

    Returns:
        int: 1 for success, -1 for fail
    """

    # Check if a VM has winlogon'ed within the last 10 seconds

    def checkWinlogon(vm_name):
        output = fractalSQLSelect(table_name="v_ms", params={"vm_name": vm_name})

        has_winlogoned = False
        if output["success"] and output["rows"]:
            has_winlogoned = (
                dateToUnix(getToday()) - output["rows"][0]["ready_to_connect"] < 10
            )

        return has_winlogoned

    # Check if a VM is a dev machine

    def checkDev(vm_name):
        output = fractalSQLSelect(table_name="v_ms", params={"vm_name": vm_name})

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

    has_winlogoned = checkWinlogon(vm_name)
    num_tries = 0

    # Return success if a winlogon has been detected within the last 10 seconds

    if has_winlogoned or checkDev(vm_name):
        return 1
    else:
        fractalLog(
            "waitForWinlogon() has detected that VM {vm_name} has not winlogon'ed. Waiting...".format(
                vm_name=vm_name
            )
        )

    # Loop 30 times at 5 second intervals, waiting for a winlogon to come in

    while not has_winlogoned:
        time.sleep(5)
        has_winlogoned = checkWinlogon(vm_name)
        num_tries += 1

        # Give up if we've waited too long

        if num_tries > 30:
            fractalLog(
                "VM {vm_name} did not winlogon after 30 tries. Giving up...".format(
                    vm_name=vm_name
                ),
                level=logging.WARNING,
            )
            return -1

    fractalLog(
        "VM {vm_name} winlogoned successfully after {} tries".format(
            vm_name=vm_name, num_tries=str(num_tries)
        ),
    )
    return 1


def installApplications(vm_name, apps):
    _, compute_client, _ = createClients()
    try:
        for app in apps:
            fractalLog(
                "installApplications() Installing {app} onto VM {vm_name}".format(
                    app=app["app_name"], vm_name=vm_name
                )
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
                "installApplications() app installation finished with message: {result}".format(
                    result=str(result.value[0].message)
                )
            )

    except Exception as e:
        fractalLog(
            "installApplications() encountered an error while installing apps onto VM {vm_name}: {error}".format(
                vm_name=vm_name, error=str(e)
            )
        )
        return -1
    return 1
