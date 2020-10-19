from app import celery_instance, db
from app.constants.config import VM_GROUP
from app.constants.http_codes import REQUEST_TIMEOUT, SUCCESS
from app.helpers.utils.azure.azure_general import createClients, getVMUser
from app.helpers.utils.azure.azure_resource_locks import lockVMAndUpdate
from app.helpers.utils.azure.azure_resource_locks import spinLock
from app.helpers.utils.azure.azure_resource_state_management import fractalVMStart
from app.helpers.utils.general.logs import fractalLog
from app.helpers.utils.general.sql_commands import fractalSQLCommit
from app.helpers.utils.general.sql_commands import fractalSQLUpdate
from app.helpers.utils.general.time import dateToUnix, getToday
from app.helpers.utils.stripe.stripe_payments import stripeChargeHourly
from app.models.hardware import OSDisk, UserVM
from app.models.logs import LoginHistory
from app.serializers.hardware import OSDiskSchema, UserVMSchema

user_vm_schema = UserVMSchema()
os_disk_schema = OSDiskSchema()


@celery_instance.task(bind=True)
def startVM(self, vm_name, resource_group=VM_GROUP):
    """Starts an Azure VM

    Args:
        vm_name (str): The name of the VM
        resource_group (str): The name of the VM's resource group

    Returns:
        json: Success/failure
    """

    if spinLock(vm_name, resource_group) < 0:
        return {"status": REQUEST_TIMEOUT}

    fractalLog(
        function="startVM",
        label=getVMUser(vm_name, resource_group),
        logs="Starting VM {vm_name} in resource group {resource_group}.".format(
            vm_name=vm_name, resource_group=resource_group
        ),
    )

    lockVMAndUpdate(vm_name=vm_name, state="STARTING", lock=True, temporary_lock=3)

    _, compute_client, _ = createClients()

    fractalVMStart(vm_name, resource_group=resource_group, s=self)

    vm = UserVM.query.get(vm_name)

    fractalLog(
        function="startVM",
        label=getVMUser(vm_name, resource_group),
        logs="VM {vm_name} successfully started.".format(vm_name=vm_name),
    )

    if vm:
        vm = user_vm_schema.dump(vm)
        return vm


@celery_instance.task(bind=True)
def restartVM(self, vm_name, resource_group=VM_GROUP):
    """Restarts an Azure VM

    Args:
        vm_name (str): The name of the VM
        resource_group (str): The name of the VM's resource group

    Returns:
        json: Success/failure
    """

    if spinLock(vm_name, resource_group) < 0:
        return {"status": REQUEST_TIMEOUT}

    fractalLog(
        function="restartVM",
        label=getVMUser(vm_name, resource_group),
        logs="Restarting VM {vm_name} in resource group {resource_group}.".format(
            vm_name=vm_name, resource_group=resource_group
        ),
    )

    lockVMAndUpdate(vm_name=vm_name, state="RESTARTING", lock=True, temporary_lock=3)

    _, compute_client, _ = createClients()

    fractalVMStart(vm_name, needs_restart=True, resource_group=resource_group, s=self)

    vm = UserVM.query.get(vm_name)

    fractalLog(
        function="restartVM",
        label=getVMUser(vm_name, resource_group),
        logs="VM {vm_name} successfully restarted.".format(vm_name=vm_name),
    )

    if vm:
        vm = user_vm_schema.dump(vm)
        return vm


@celery_instance.task(bind=True)
def deallocateVM(self, vm_name, resource_group=VM_GROUP):
    """Deallocates an Azure VM

    Args:
        vm_name (str): The name of the VM
        resource_group (str): The name of the VM's resource group

    Returns:
        json: Success/failure
    """

    if spinLock(vm_name, resource_group) < 0:
        return {"status": REQUEST_TIMEOUT}

    fractalLog(
        function="deallocateVM",
        label=getVMUser(vm_name, resource_group),
        logs="Deallocating VM {vm_name}.".format(vm_name=vm_name),
    )

    lockVMAndUpdate(vm_name=vm_name, state="DEALLOCATING", lock=True, temporary_lock=3)

    _, compute_client, _ = createClients()

    async_vm_deallocate = compute_client.virtual_machines.deallocate(resource_group, vm_name)
    async_vm_deallocate.wait()

    vm = UserVM.query.get(vm_name)

    lockVMAndUpdate(vm_name=vm_name, state="DEALLOCATED", lock=False, temporary_lock=0)

    fractalLog(
        function="deallocateVM",
        label=getVMUser(vm_name, resource_group),
        logs="VM {vm_name} successfully deallocated.".format(vm_name=vm_name),
    )

    if vm:
        vm = user_vm_schema.dump(vm)
        return vm


@celery_instance.task
def pingHelper(available, vm_ip, version=None):
    """Stores ping timestamps in the v_ms table and tracks number of hours used

    Args:
        available (bool): True if VM is not being used, False otherwise
        vm_ip (str): VM IP address

    Returns:
        json: Success/failure
    """

    # Retrieve VM data based on VM IP

    vm_info = UserVM.query.filter_by(ip=vm_ip).first()

    if vm_info:
        username = vm_info.user_id
    else:
        raise Exception(f"No virtual machine with IP {vm_ip}")

    disk = OSDisk.query.filter_by(user_id=username).first()

    fractalSQLCommit(db, fractalSQLUpdate, disk, {"last_pinged": dateToUnix(getToday())})

    # Update disk version

    if version:
        fractalSQLCommit(db, fractalSQLUpdate, disk, {"version": version})

    # Define states where we don't change the VM state

    intermediate_states = ["STOPPING", "DEALLOCATING", "ATTACHING"]

    # Detect and handle disconnect event

    if vm_info.state == "RUNNING_UNAVAILABLE" and available:

        # Add pending charge if the user is an hourly subscriber

        stripeChargeHourly(username)

        # Add logoff event to timetable

        log = LoginHistory(
            user_id=username,
            action="logoff",
            timestamp=dateToUnix(getToday()),
        )

        fractalSQLCommit(db, lambda db, x: db.session.add(x), log)

        fractalLog(
            function="pingHelper",
            label=str(username),
            logs="{username} just disconnected from their cloud PC".format(username=username),
        )

    # Detect and handle logon event

    if vm_info.state == "RUNNING_AVAILABLE" and not available:

        # Add logon event to timetable

        log = LoginHistory(
            user_id=username,
            action="logon",
            timestamp=dateToUnix(getToday()),
        )

        fractalSQLCommit(db, lambda db, x: db.session.add(x), log)

        fractalLog(
            function="pingHelper",
            label=str(username),
            logs="{username} just connected to their cloud PC".format(username=username),
        )

    # Change VM states accordingly

    if vm_info.state not in intermediate_states and not available:
        lockVMAndUpdate(
            vm_name=vm_info.vm_id,
            state="RUNNING_UNAVAILABLE",
            lock=True,
            temporary_lock=0,
        )

    if vm_info.state not in intermediate_states and available:
        lockVMAndUpdate(
            vm_name=vm_info.vm_id,
            state="RUNNING_AVAILABLE",
            lock=False,
            temporary_lock=None,
        )

    return {"status": SUCCESS}
