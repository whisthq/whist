from app import *
from app.helpers.utils.azure.azure_general import *
from app.helpers.utils.azure.azure_resource_state_management import *
from app.helpers.utils.azure.azure_resource_locks import *


@celery_instance.task(bind=True)
def deleteVM(self, vm_name, delete_disk):
    """Deletes an Azure VM

    Args:
        vm_name (str): The name of the VM
        delete_disk (bool): True to delete the disk attached to the VM, False otherwise

    Returns:
        json: Success/failure
    """

    if spinLock(vm_name) < 0:
        return {"status": REQUEST_TIMEOUT}

    fractalLog(
        function="deleteVM",
        label=str(vm_name),
        logs="Beginning to delete VM {vm_name}. Goodbye, {vm_name}!".format(
            vm_name=vm_name
        ),
    )

    lockVMAndUpdate(vm_name=vm_name, state="DELETING", lock=True, temporary_lock=10)

    _, compute_client, network_client = createClients()
    vnet_name, subnet_name, ip_name, nic_name = (
        vm_name + "_vnet",
        vm_name + "_subnet",
        vm_name + "_ip",
        vm_name + "_nic",
    )

    hr = 1

    # get VM info based on name
    virtual_machine = createVMInstance(vm_name)
    os_disk_name = virtual_machine.storage_profile.os_disk.name

    # step 1, deallocate the VM
    try:
        async_vm_deallocate = compute_client.virtual_machines.deallocate(
            os.getenv("VM_GROUP"), vm_name
        )
        async_vm_deallocate.wait()

        fractalLog(
            function="deleteVM",
            label=str(vm_name),
            logs="VM {vm_name} deallocated".format(vm_name=vm_name),
        )

        time.sleep(60)
    except Exception as e:
        fractalLog(
            function="deleteVM",
            label=str(vm_name),
            logs="Error deallocating VM {vm_name}: {error}".format(
                vm_name=vm_name, error=str(e)
            ),
            level=logging.ERROR,
        )
        hr = -1

    # step 2, detach the IP
    try:
        subnet_obj = network_client.subnets.get(
            resource_group_name=os.getenv("VM_GROUP"),
            virtual_network_name=vnet_name,
            subnet_name=subnet_name,
        )
        # configure network interface parameters.
        params = {
            "location": virtual_machine.location,
            "ip_configurations": [
                {
                    "name": ip_name,
                    "subnet": {"id": subnet_obj.id},
                    # None: Disassociate;
                    "public_ip_address": None,
                }
            ],
        }
        # use method create_or_update to update network interface configuration.
        async_ip_detach = network_client.network_interfaces.create_or_update(
            resource_group_name=os.getenv("VM_GROUP"),
            network_interface_name=nic_name,
            parameters=params,
        )
        async_ip_detach.wait()

        fractalLog(
            function="deleteVM",
            label=str(vm_name),
            logs="IP {ip_name} detached".format(ip_name=ip_name),
        )
    except Exception as e:
        fractalLog(
            function="deleteVM",
            label=str(vm_name),
            logs="Error detaching IP {ip_name}: {error}".format(
                ip_name=ip_name, error=str(e)
            ),
            level=logging.ERROR,
        )
        hr = -1

    # step 3, delete the VM
    try:
        async_vm_delete = compute_client.virtual_machines.delete(
            os.getenv("VM_GROUP"), vm_name
        )
        async_vm_delete.wait()

        fractalSQLDelete(table_name="v_ms", params={"vm_name": vm_name})

        fractalLog(
            function="deleteVM",
            label=str(vm_name),
            logs="VM {vm_name} deleted".format(vm_name=vm_name),
            level=logging.ERROR,
        )
    except Exception as e:
        fractalLog(
            function="deleteVM",
            label=str(vm_name),
            logs="Error deleting VM {vm_name}: {error}".format(
                vm_name=vm_name, error=str(e)
            ),
            level=logging.ERROR,
        )
        hr = -1

    # step 4, delete the IP
    try:
        async_ip_delete = network_client.public_ip_addresses.delete(
            os.getenv("VM_GROUP"), ip_name
        )
        async_ip_delete.wait()

        fractalLog(
            function="deleteVM",
            label=str(vm_name),
            logs="IP {ip_name} deleted".format(ip_name=ip_name),
        )
    except Exception as e:
        fractalLog(
            function="deleteVM",
            label=str(vm_name),
            logs="Error deleting IP {ip_name}: {error}".format(
                ip_name=ip_name, error=str(e)
            ),
            level=logging.ERROR,
        )
        hr = -1

    # step 4, delete the NIC
    try:
        async_nic_delete = network_client.network_interfaces.delete(
            os.getenv("VM_GROUP"), nic_name
        )
        async_nic_delete.wait()

        fractalLog(
            function="deleteVM",
            label=str(vm_name),
            logs="Nic {nic_name} deleted".format(nic_name=nic_name),
        )
    except Exception as e:
        fractalLog(
            function="deleteVM",
            label=str(vm_name),
            logs="Error deleting Nic {nic_name}: {error}".format(
                nic_name=nic_name, error=str(e)
            ),
            level=logging.ERROR,
        )
        hr = -1

    # step 5, delete the Vnet
    try:
        async_vnet_delete = network_client.virtual_networks.delete(
            os.getenv("VM_GROUP"), vnet_name
        )
        async_vnet_delete.wait()

        fractalLog(
            function="deleteVM",
            label=str(vm_name),
            logs="Vnet {vnet_name} deleted".format(vnet_name=vnet_name),
        )
    except Exception as e:
        fractalLog(
            function="deleteVM",
            label=str(vm_name),
            logs="Error deleting Vnet {vnet_name}: {error}".format(
                vnet_name=vnet_name, error=str(e)
            ),
            level=logging.ERROR,
        )
        hr = -1

    if delete_disk:
        # step 6, delete the OS disk
        try:
            os_disk_delete = compute_client.disks.delete(
                os.getenv("VM_GROUP"), os_disk_name
            )
            os_disk_delete.wait()

            fractalLog(
                function="deleteVM",
                label=str(vm_name),
                logs="OS disk {disk_name} deleted".format(disk_name=os_disk_name),
            )
        except Exception as e:
            fractalLog(
                function="deleteVM",
                label=str(vm_name),
                logs="Error deleting OS disk {disk_name}: {error}".format(
                    disk_name=os_disk_name, error=str(e)
                ),
                level=logging.ERROR,
            )
            hr = -1

    return {"status": SUCCESS} if hr > 0 else {"status": PARTIAL_CONTENT}
