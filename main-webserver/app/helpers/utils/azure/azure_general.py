from app import *


def createClients():
    """Creates Azure management clients

    This function is used to access the resource management, compute management, and network management clients,
    which are modules in the Azure Python SDK.

    Returns:
    ResourceManagementClient, ComputeManagementClient, NetworkManagementClient

   """
    subscription_id = os.getenv("AZURE_SUBSCRIPTION_ID")
    credentials = ServicePrincipalCredentials(
        client_id=os.getenv("AZURE_CLIENT_ID"),
        secret=os.getenv("AZURE_CLIENT_SECRET"),
        tenant=os.getenv("AZURE_TENANT_ID"),
    )
    r = ResourceManagementClient(credentials, subscription_id)
    c = ComputeManagementClient(credentials, subscription_id)
    n = NetworkManagementClient(credentials, subscription_id)
    return r, c, n


def createVMName():
    """Generates a unique name for a vm

    Returns:
        str: The generated name
    """
    output = fractalSQLSelect("v_ms", {})
    old_names = []
    if output["rows"]:
        old_names = [vm["vm_name"] for vm in output["rows"]]
    vm_name = genHaiku(1)[0]

    while vm_name in old_names:
        vm_name = genHaiku(1)[0]

    return vm_name


def createDiskName():
    """Generates a unique name for a disk

    Returns:
        str: The generated name
    """
    output = fractalSQLSelect("disks", {})
    old_names = []
    if output["rows"]:
        old_names = [disk["disk_name"] for disk in output["rows"]]
    disk_name = genHaiku(1)[0] + "_disk"

    while disk_name in old_names:
        disk_name = genHaiku(1)[0] + "_disk"

    return disk_name


def createVMInstance(vm_name, resource_group=os.getenv("VM_GROUP")):
    """Retrieves information about the model view or the instance view of an Azure virtual machine

    Parameters:
    vm_name (str): The name of the VM to retrieve

    Returns:
    VirtualMachine: The instance view of the virtual machine
   """

    _, compute_client, _ = createClients()
    try:
        virtual_machine = compute_client.virtual_machines.get(
            resource_group_name=resource_group, vm_name=vm_name
        )
        return virtual_machine
    except Exception as e:
        fractalLog(
            function="createVMInstance",
            label=str(vm_name),
            logs="Error creating VM instance: {error}".format(error=str(e)),
        )

        return None


def getVMIP(vm_name, resource_group=os.getenv("VM_GROUP")):
    """Gets the IP address for a vm

    Args:
        vm_name (str): The name of the vm

    Returns:
        str: The ipv4 address
    """
    try:
        fractalLog(
            function="getVMIP",
            label=str(vm_name),
            logs="Getting IP for {vm_name} in resource group {resource_group}".format(
                vm_name=vm_name, resource_group=resource_group
            ),
        )

        vm = createVMInstance(vm_name, resource_group)

        _, _, network_client = createClients()
        ni_reference = vm.network_profile.network_interfaces[0]
        ni_reference = ni_reference.id.split("/")
        ni_group = ni_reference[4]
        ni_name = ni_reference[8]

        net_interface = network_client.network_interfaces.get(ni_group, ni_name)
        ip_reference = net_interface.ip_configurations[0].public_ip_address
        ip_reference = ip_reference.id.split("/")
        ip_group = ip_reference[4]
        ip_name = ip_reference[8]

        public_ip = network_client.public_ip_addresses.get(ip_group, ip_name)
        return public_ip.ip_address
    except Exception as e:
        return str(e)


def resourceGroupToTable(resource_group):
    try:
        table_mapping = {
            "Fractal": "v_ms",
            "FractalStaging": "v_ms",
            "FractalProtocolCI": "VMs_FractalProtocolCI",
        }

        return table_mapping[resource_group]
    except:
        return None


def getVMUser(vm_name, resource_group=os.getenv("VM_GROUP")):
    output = fractalSQLSelect(
        table_name=resourceGroupToTable(resource_group), params={"vm_name": vm_name}
    )

    if output["success"] and output["rows"]:
        return str(output["rows"][0]["username"])

    return "None"


def getDiskUser(disk_name):
    output = fractalSQLSelect(table_name="disks", params={"disk_name": disk_name})

    if output["success"] and output["rows"]:
        return str(output["rows"][0]["username"])

    return "None"
