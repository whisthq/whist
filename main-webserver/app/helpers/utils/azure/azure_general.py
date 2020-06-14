from app import *
from app.helpers.utils.general.sql_commands import *
from app.helpers.utils.general.tokens import *


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


def createVMInstance(vm_name):
    """Retrieves information about the model view or the instance view of an Azure virtual machine

    Parameters:
    vm_name (str): The name of the VM to retrieve

    Returns:
    VirtualMachine: The instance view of the virtual machine
   """
    _, compute_client, _ = createClients()
    try:
        virtual_machine = compute_client.virtual_machines.get(
            os.environ.get("VM_GROUP"), vm_name
        )
        return virtual_machine
    except:
        return None


def getVMIP(vm_name):
    """Gets the IP address for a vm

    Args:
        vm_name (str): The name of the vm

    Returns:
        str: The ipv4 address
    """
    vm = createVMInstance(vm_name)

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
