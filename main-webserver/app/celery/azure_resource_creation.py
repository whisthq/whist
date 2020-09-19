from app import *
from app.helpers.utils.azure.azure_general import *
from app.helpers.utils.azure.azure_resource_creation import *
from app.helpers.utils.azure.azure_resource_state_management import *
from app.helpers.utils.azure.azure_resource_locks import *
from app.helpers.utils.general.crypto import *

from app.models.hardware import *
from app.serializers.hardware import *

user_vm_schema = UserVMSchema()
os_disk_schema = OSDiskSchema()


@celery_instance.task(bind=True)
def createVM(
    self, vm_size, location, operating_system, admin_password=None, resource_group=VM_GROUP
):
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
        state="PENDING", meta={"msg": "Creating NIC for VM {vm_name}".format(vm_name=vm_name)}
    )

    # Create network resources

    nic = createNic(vm_name, location, 0, resource_group=resource_group)

    if not nic:
        self.update_state(
            state="FAILURE",
            meta={"msg": "NIC failed to create for VM {vm_name}".format(vm_name=vm_name)},
        )

        return

    vm_parameters = createVMParameters(
        vm_name, nic.id, vm_size, location, operating_system, admin_password=admin_password
    )

    self.update_state(
        state="PENDING",
        meta={"msg": "Command to create VM {vm_name} started".format(vm_name=vm_name)},
    )

    # Create VM

    async_vm_creation = compute_client.virtual_machines.create_or_update(
        resource_group, vm_name, vm_parameters["params"]
    )

    async_vm_creation.wait()

    self.update_state(state="PENDING", meta={"msg": "VM {} created successfully".format(vm_name)})

    time.sleep(30)

    # Start VM

    async_vm_start = compute_client.virtual_machines.start(resource_group, vm_name)

    async_vm_start.wait()

    time.sleep(30)

    # Store VM in v_ms database

    vm_instance = createVMInstance(vm_name, resource_group=resource_group)
    disk_name = vm_instance.storage_profile.os_disk.name

    # Generate private key for AES

    private_key = generatePrivateKey()

    self.update_state(
        state="PENDING",
        meta={
            "msg": "VM {vm_name} with disk {disk_name} started".format(
                vm_name=vm_name, disk_name=disk_name
            )
        },
    )

    ip_address = getVMIP(vm_name, resource_group)

    vm = UserVM(
        vm_id=vm_name,
        ip=ip_address,
        state="CREATING",
        location=location,
        os=operating_system,
        temporary_lock=0,
        disk_id=disk_name,
        lock=False,
    )

    disk = OSDisk(
        disk_id=disk_name,
        location=location,
        os=operating_system,
        allow_autoupdate=True,
        has_dedicated_vm=False,
        branch="master",
        using_stun=False,
        disk_size=120,
        state="TO_BE_DELETED",
    )

    add_disk = fractalSQLCommit(db, lambda db, x: db.session.add(x), disk)
    add_vm = fractalSQLCommit(db, lambda db, x: db.session.add(x), vm)

    if add_disk and add_vm:
        # Install NVIDIA GRID driver

        fractalLog(
            function="createVM", label=str(vm_name), logs="Started to install NVIDIA extension"
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

        time.sleep(30)

        self.update_state(
            state="PENDING", meta={"msg": "VM {} installing NVIDIA extension".format(vm_name)}
        )

        async_vm_extension = compute_client.virtual_machine_extensions.create_or_update(
            resource_group, vm_name, extension_parameters["vm_extension_name"], extension_parameters
        )

        async_vm_extension.wait()

        fractalLog(function="createVM", label=str(vm_name), logs="NVIDIA extension done installing")

        lockVMAndUpdate(
            vm_name, "RUNNING_AVAILABLE", False, temporary_lock=None, resource_group=resource_group
        )

        fractalLog(
            function="createVM",
            label=str(vm_name),
            logs="{operating_system} VM successfully created in {location}".format(
                operating_system=operating_system, location=location
            ),
        )

        # Fetch VM columns from SQL and return
        vm = UserVM.query.get(vm_name)
        vm = user_vm_schema.dump(vm)

        return vm

    else:
        fractalLog(function="createVM", label=str(vm_name), logs="SQL insertion unsuccessful")

        self.update_state(
            state="FAILURE",
            meta={"msg": "Error inserting VM {vm_name} and disk into SQL".format(vm_name=vm_name)},
        )

        return None


@celery_instance.task(bind=True)
def cloneDisk(self, username, location, operating_system, branch, apps=[], resource_group=VM_GROUP):
    """Clones a Windows/Linux Fractal disk

    Args:
        username (str): The size of the vm to create
        location (str): The Azure region (eastus, northcentralus, southcentralus)
        vm_size (str): Azure VM type (Standard_NV6_Promo)
        operating_system (str): Windows or Linux
        apps (list): List of apps to pre-install

    Returns:
        json: Cloned disk metadata
    """

    fractalLog(
        function="cloneDisk",
        label=str(username),
        logs="Starting to clone {operating_system} disk in {location}".format(
            operating_system=operating_system, location=location
        ),
    )

    disk_name = createDiskName()

    fractalLog(
        function="cloneDisk",
        label=str(username),
        logs="Cloned disk will be called {disk_name}".format(disk_name=disk_name),
    )

    _, compute_client, _ = createClients()

    # Start disk creation

    try:
        # Retrieve Fractal template disk

        location_disk_map = {
            "eastus": "Fractal_Disk_Eastus",
            "southcentralus": "Fractal_Disk_Southcentralus",
            "northcentralus": "Fractal_Disk_Northcentralus",
        }

        try:
            original_disk_name = location_disk_map[location]
        except:
            return {
                "status": BAD_REQUEST,
                "error": "Location {location} is not valid. Valid locations are eastus, southcentralus, and northcentralus".format(
                    location=location
                ),
            }

        if operating_system == "Linux":
            original_disk_name = original_disk_name + "_Linux"

        original_disk = compute_client.disks.get("Fractal", original_disk_name)

        fractalLog(
            function="cloneDisk",
            label=str(username),
            logs="Sending Azure command to create disk {disk_name}".format(disk_name=disk_name),
        )

        async_disk_creation = compute_client.disks.create_or_update(
            resource_group,
            disk_name,
            {
                "location": location,
                "creation_data": {
                    "create_option": DiskCreateOption.copy,
                    "source_resource_id": original_disk.id,
                    "managed_disk": {"storage_account_type": "StandardSSD_LRS"},
                },
            },
        )

        async_disk_creation.wait()

        fractalLog(
            function="cloneDisk",
            label=str(username),
            logs="Disk {disk_name} created successfully in resource group {resource_group}".format(
                disk_name=disk_name, resource_group=resource_group
            ),
        )

        try:
            disk = OSDisk(
                disk_id=disk_name,
                user_id=username,
                location=location,
                os=operating_system,
                allow_autoupdate=True,
                has_dedicated_vm=False,
                branch=branch,
                using_stun=False,
                disk_size=120,
                state="ACTIVE",
            )

            fractalSQLCommit(db, lambda db, x: db.session.add(x), disk)
        except:
            return {"status": BAD_REQUEST, "disk_name": disk_name}

        return {"status": SUCCESS, "disk_name": disk_name}

    except Exception as e:
        fractalLog(
            function="cloneDisk",
            label=str(username),
            logs="Disk cloning failed with error: {error}".format(error=str(e)),
            level=logging.CRITICAL,
        )

        async_disk_deletion = compute_client.disks.delete(resource_group, disk_name)
        async_disk_deletion.wait()

        return {"status": BAD_REQUEST, "error": str(e)}


@celery_instance.task(bind=True)
def createDisk(
    self, disk_size, username, location, resource_group=VM_GROUP, operating_system="Windows"
):
    """Creates an empty Windows Azure managed disk

    Args:
        disk_size (str): Size of disk in GB
        username (str): The size of the vm to create
        location (str): The Azure region (eastus, northcentralus, southcentralus)
        resource_group (str): Azure resource group name

    Returns:
        json: New disk name
    """

    disk_name = createDiskName()

    fractalLog(
        function="createDisk",
        label=str(username),
        logs="New disk will be called {disk_name}".format(disk_name=disk_name),
    )

    _, compute_client, _ = createClients()
    async_disk_creation = compute_client.disks.create_or_update(
        resource_group,
        disk_name,
        {
            "location": location,
            "disk_size_gb": disk_size,
            "creation_data": {"create_option": DiskCreateOption.empty},
        },
    )

    async_disk_creation.wait()

    disk = SecondaryDisk(
        disk_id=disk_name,
        user_id=username,
        location=location,
        disk_size=disk_size,
        os=operating_system,
    )

    fractalSQLCommit(db, lambda db, x: db.session.add(x), disk)

    fractalLog(
        function="createDisk",
        label=str(username),
        logs="{disk_name} was created successfully as a secondary disk".format(disk_name=disk_name),
    )

    return {"status": 200, "disk_name": disk_name}
