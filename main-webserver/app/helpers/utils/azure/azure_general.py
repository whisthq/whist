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
