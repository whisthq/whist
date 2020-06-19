from app.utils import *
from app import *
from app.logger import *


def createClients():
    """Creates Azure management clients

    This function is used to access the resource management, compute management, and network management clients, which are used to access the Azure VM API

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
