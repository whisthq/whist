from app import *
from app.helpers.utils.azure.azure_general import *
from app.helpers.utils.azure.azure_resource_creation import *
from app.helpers.utils.azure.azure_resource_state_management import *


@celery.task(bind=True)
def createVM(self, vm_size, location, operating_system, admin_password=None):
    """Creates a windows vm of size vm_size in Azure region location

    Args:
        vm_size (str): The size of the vm to create
        location (str): The Azure region (eastus, northcentralus, southcentralus)
        operating_system (str): Windows or Linux
        admin_password (str): Custom admin password, will use default password if None

    Returns:
        json: The json representing the vm in the v_ms sql table
    """
    fractalLog(
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
    vm_name = createVMName()

    nic = createNic(vm_name, location, 0)

    self.update_state(
        state="PENDING",
        meta={"msg": "Creating NIC for VM {vm_name}".format(vm_name=vm_name)},
    )

    if not nic:
        self.update_state(
            state="FAILURE",
            meta={"msg": "NIC failed to create for VM {vm_name}".format(vm_name)},
        )

        return

    vmParameters = createVMParameters(
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

    async_vm_creation = compute_client.virtual_machines.create_or_update(
        os.getenv("VM_GROUP"), vm_name, vmParameters["params"]
    )

    async_vm_creation.wait()

    self.update_state(
        state="PENDING", meta={"msg": "VM {} created successfully".format(vm_name)},
    )

    time.sleep(10)

    self.update_state(
        state="PENDING", meta={"msg": "VM {} starting".format(vm_name)},
    )

    async_vm_start = compute_client.virtual_machines.start(
        os.getenv("VM_GROUP"), vm_name
    )
    sendDebug(ID, "Waiting on async_vm_start")
    async_vm_start.wait()

    self.update_state(
        state="PENDING", meta={"msg": "VM {} started".format(vm_name)},
    )

    time.sleep(30)

    sendInfo(ID, "The VM created is called {}".format(vm_name))

    fractalVMStart(vm_name, needs_winlogon=False, s=self)

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
        meta={"msg": "VM {} installing NVIDIA extension".format(vm_name)},
    )

    async_vm_extension = compute_client.virtual_machine_extensions.create_or_update(
        os.getenv("VM_GROUP"),
        vm_name,
        extension_parameters["vm_extension_name"],
        extension_parameters,
    )

    sendDebug(ID, "Waiting on async_vm_extension")
    async_vm_extension.wait()

    self.update_state(
        state="PENDING",
        meta={"msg": "VM {} successfully installed NVIDIA extension".format(vm_name)},
    )

    vm = getVM(vm_name)
    vm_ip = getIP(vm)
    updateVMIP(vm_name, vm_ip)
    updateVMState(vm_name, "RUNNING_AVAILABLE")
    updateVMLocation(vm_name, location)
    updateVMOS(vm_name, operating_system)

    sendInfo(ID, "SUCCESS: VM {} created and updated".format(vm_name))

    return fetchVMCredentials(vm_name)
