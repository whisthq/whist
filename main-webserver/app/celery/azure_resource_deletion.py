from app import *
from app.helpers.utils.azure.azure_general import *
from app.helpers.utils.azure.azure_resource_creation import *
from app.helpers.utils.azure.azure_resource_state_management import *
from app.helpers.utils.azure.azure_resource_locks import *


@celery_instance.task(bind=True)
def createVM(self, vm_name, delete_disk):
    """Deletes an Azure VM

    Args:
        vm_name (str): The name of the VM
        delete_disk (bool): True to delete the disk attached to the VM, False otherwise

    Returns:
        json: Success/failure
    """

    _, compute_client, network_client = createClients()
    vnetName, subnetName, ipName, nicName = (
        vm_name + "_vnet",
        vm_name + "_subnet",
        vm_name + "_ip",
        vm_name + "_nic",
    )

    hr = 1

    # get VM info based on name
    virtual_machine = createVMInstance(name)
    os_disk_name = virtual_machine.storage_profile.os_disk.name

    # step 1, deallocate the VM
    try:
        async_vm_deallocate = compute_client.virtual_machines.deallocate(
            os.getenv("VM_GROUP"), vm_name
        )
        async_vm_deallocate.wait()
        # wait a whole minute to ensure it deallocated properly
        time.sleep(60)
    except Exception as e:
        sendError(ID, str(e))
        hr = -1

    # step 2, detach the IP
    try:
        subnet_obj = network_client.subnets.get(
            resource_group_name=os.getenv("VM_GROUP"),
            virtual_network_name=vnetName,
            subnet_name=subnetName,
        )
        # configure network interface parameters.
        params = {
            "location": virtual_machine.location,
            "ip_configurations": [
                {
                    "name": ipName,
                    "subnet": {"id": subnet_obj.id},
                    # None: Disassociate;
                    "public_ip_address": None,
                }
            ],
        }
        # use method create_or_update to update network interface configuration.
        async_ip_detach = network_client.network_interfaces.create_or_update(
            resource_group_name=os.getenv("VM_GROUP"),
            network_interface_name=nicName,
            parameters=params,
        )
        async_ip_detach.wait()
    except Exception as e:
        hr = -1

    # step 3, delete the VM
    try:
        async_vm_delete = compute_client.virtual_machines.delete(
            os.getenv("VM_GROUP"), vm_name
        )
        async_vm_delete.wait()

        fractalSQLDelete(table_name="v_ms", params={"vm_name": vm_name})
    except Exception as e:
        hr = -1

    # step 4, delete the IP
    try:
        async_ip_delete = network_client.public_ip_addresses.delete(
            os.getenv("VM_GROUP"), ipName
        )
        async_ip_delete.wait()
    except Exception as e:
        hr = -1

    # step 4, delete the NIC
    try:
        async_nic_delete = network_client.network_interfaces.delete(
            os.getenv("VM_GROUP"), nicName
        )
        async_nic_delete.wait()
    except Exception as e:
        hr = -1

    # step 5, delete the Vnet
    try:
        async_vnet_delete = network_client.virtual_networks.delete(
            os.getenv("VM_GROUP"), vnetName
        )
        async_vnet_delete.wait()
    except Exception as e:
        hr = -1

    if delete_disk:
        # step 6, delete the OS disk -- not needed anymore (OS disk swapping)
        try:
            os_disk_delete = compute_client.disks.delete(
                os.getenv("VM_GROUP"), os_disk_name
            )
            os_disk_delete.wait()
        except Exception as e:
            hr = -1

    return hr
