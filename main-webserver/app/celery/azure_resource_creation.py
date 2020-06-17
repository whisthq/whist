from app import *
from app.helpers.utils.azure.azure_general import *
from app.helpers.utils.azure.azure_resource_creation import *
from app.helpers.utils.azure.azure_resource_state_management import *
from app.helpers.utils.azure.azure_resource_locks import *


@celery_instance.task(bind=True)
def createVM(self, vm_size, location, operating_system, admin_password=None):
    """Creates a Windows/Linux VM of size vm_size in Azure region location

    Args:
        vm_size (str): The size of the vm to create
        location (str): The Azure region (eastus, northcentralus, southcentralus)
        operating_system (str): Windows or Linux
        admin_password (str): Custom admin password, will use default password if None

    Returns:
        json: The json representing the vm in the v_ms sql table
    """

    fractalLog(
        function="createVM",
        label="None",
        logs="Creating VM of size {vm_size}, location {location}, operating system {operating_system}".format(
            vm_size=vm_size, location=location, operating_system=operating_system
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

    # Generate a random VM name

    _, compute_client, _ = createClients()
    vm_name = createVMName()

    self.update_state(
        state="PENDING",
        meta={"msg": "Creating NIC for VM {vm_name}".format(vm_name=vm_name)},
    )

    # Create network resources

    nic = createNic(vm_name, location, 0)

    if not nic:
        self.update_state(
            state="FAILURE",
            meta={"msg": "NIC failed to create for VM {vm_name}".format(vm_name)},
        )

        return

    vm_parameters = createVMParameters(
        vm_name,
        nic.id,
        vm_size,
        location,
        operating_system,
        admin_password=admin_password,
    )

    self.update_state(
        state="PENDING",
        meta={"msg": "Command to create VM {vm_name} started".format(vm_name=vm_name)},
    )

    # Create VM

    async_vm_creation = compute_client.virtual_machines.create_or_update(
        os.getenv("VM_GROUP"), vm_name, vm_parameters["params"]
    )

    async_vm_creation.wait()

    self.update_state(
        state="PENDING", meta={"msg": "VM {} created successfully".format(vm_name)},
    )

    time.sleep(10)

    # Start VM

    async_vm_start = compute_client.virtual_machines.start(
        os.getenv("VM_GROUP"), vm_name
    )

    async_vm_start.wait()

    time.sleep(30)

    # Store VM in v_ms database

    vm_instance = createVMInstance(vm_name)
    disk_name = vm_instance.storage_profile.os_disk.name

    self.update_state(
        state="PENDING",
        meta={
            "msg": "VM {vm_name} with disk {disk_name} started".format(
                vm_name=vm_name, disk_name=disk_name
            )
        },
    )

    ip_address = getVMIP(vm_name)
    fractalSQLInsert(
        table_name="v_ms",
        params={
            "vm_name": vm_name,
            "ip": ip_address,
            "state": "CREATING",
            "location": location,
            "dev": False,
            "os": operating_system,
            "lock": True,
            "disk_name": disk_name,
        },
    )

    fractalSQLInsert(
        table_name="disks",
        params={
            "disk_name": disk_name,
            "location": location,
            "state": "TO_BE_DELETED",
            "disk_size": 120,
        },
    )

    # Make sure VM is started

    fractalVMStart(vm_name, needs_winlogon=False, s=self)

    # Install NVIDIA GRID driver

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

    time.sleep(30)

    self.update_state(
        state="PENDING",
        meta={"msg": "VM {} installing NVIDIA extension".format(vm_name)},
    )

    async_vm_extension = compute_client.virtual_machine_extensions.create_or_update(
        os.getenv("VM_GROUP"),
        vm_name,
        extension_parameters["vm_extension_name"],
        extension_parameters,
    )

    fractalLog(
        function="createVM",
        label=str(vm_name),
        logs="Started to install NVIDIA extension",
    )

    async_vm_extension.wait()

    fractalLog(
        function="createVM", label=str(vm_name), logs="NVIDIA extension done installing"
    )

    # Fetch VM columns from SQL and return

    output = fractalSQLSelect("v_ms", {"vm_name": vm_name})

    lockVMAndUpdate(vm_name, "RUNNING_AVAILABLE", False, temporary_lock=None)

    if output["success"] and output["rows"]:
        fractalLog(
            function="createVM",
            label="VM {vm_name}".format(vm_name),
            logs="{operating_system} VM successfully created in {location}".format(
                operating_system=operating_system, location=location
            ),
        )
        return output["rows"][0]
    return None
