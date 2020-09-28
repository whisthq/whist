from app import *
from app.helpers.utils.azure.azure_general import *
import secrets


def generatePrivateKey():
    return secrets.token_hex(16)


def createNic(vm_name, location, tries, resource_group=None):
    """Creates a network id

    Args:
        name (str): Name of the vm
        location (str): The azure region
        tries (int): The current number of tries

    Returns:
        dict: The network id object
    """

    resource_group = VM_GROUP if not resource_group else resource_group

    _, _, network_client = createClients()
    vnet_name, subnet_name, ip_name, nic_name = (
        vm_name + "_vnet",
        vm_name + "_subnet",
        vm_name + "_ip",
        vm_name + "_nic",
    )
    try:
        async_vnet_creation = network_client.virtual_networks.create_or_update(
            resource_group,
            vnet_name,
            {"location": location, "address_space": {"address_prefixes": ["10.0.0.0/16"]}},
        )
        async_vnet_creation.wait()

        # Create Subnet
        async_subnet_creation = network_client.subnets.create_or_update(
            resource_group, vnet_name, subnet_name, {"address_prefix": "10.0.0.0/24"}
        )
        subnet_info = async_subnet_creation.result()

        # Create public IP address
        public_ip_addess_params = {"location": location, "public_ip_allocation_method": "Static"}
        creation_result = network_client.public_ip_addresses.create_or_update(
            resource_group, ip_name, public_ip_addess_params
        )

        public_ip_address = network_client.public_ip_addresses.get(resource_group, ip_name)

        # Create NIC
        async_nic_creation = network_client.network_interfaces.create_or_update(
            resource_group,
            nic_name,
            {
                "location": location,
                "ip_configurations": [
                    {
                        "name": ip_name,
                        "public_ip_address": public_ip_address,
                        "subnet": {"id": subnet_info.id},
                    }
                ],
            },
        )

        return async_nic_creation.result()
    except Exception as e:
        if tries < 5:
            fractalLog(
                function="createNic",
                label=str(vm_name),
                logs="NIC creation encountered a retryable error: {error}".format(error=str(e)),
            )

            time.sleep(3)
            return createNic(vm_name, location, tries + 1)
        else:
            return None


def createVMParameters(
    vm_name, nic_id, vm_size, location, operating_system="Windows", admin_password=None
):
    """Creates VM creation metadata and stores the VM in database

    Parameters:
    vm_name (str): The name of the VM to add
    nic_id (str): The vm's network interface ID
    vm_size (str): The type of vm in terms of specs(default is NV6)
    location (str): The Azure region of the vm
    operating_system (str): Windows or Linux
    admin_password (str): The admin password (default is set in .env)

    Returns:
    json: Parameters that will be used in Azure sdk
    """

    # Operating system parameters

    vm_reference = (
        {
            "publisher": "MicrosoftWindowsDesktop",
            "offer": "Windows-10",
            "sku": "rs5-pro",
            "version": "latest",
        }
        if operating_system == "Windows"
        else {
            "publisher": "Canonical",
            "offer": "UbuntuServer",
            "sku": "18.04-LTS",
            "version": "latest",
        }
    )

    # Set computer name, admin username, and admin password (for autologin and RDP)

    admin_password = VM_PASSWORD if not admin_password else admin_password

    os_profile = {
        "computer_name": vm_name,
        "admin_username": "Fractal" if operating_system == "Windows" else "fractal",
        "admin_password": admin_password,
    }

    return {
        "params": {
            "location": location,
            "os_profile": os_profile,
            "hardware_profile": {"vm_size": vm_size},
            "storage_profile": {
                "image_reference": {
                    "publisher": vm_reference["publisher"],
                    "offer": vm_reference["offer"],
                    "sku": vm_reference["sku"],
                    "version": vm_reference["version"],
                },
                "os_disk": {"os_type": operating_system, "create_option": "FromImage"},
            },
            "network_profile": {"network_interfaces": [{"id": nic_id}]},
        },
        "vm_name": vm_name,
    }
