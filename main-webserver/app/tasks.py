from app import *
from .helperFuncs import *
from msrest.exceptions import ClientException


@celery.task(bind=True)
def createVM(self, vm_size, location):
    _, compute_client, _ = createClients()
    vmName = genVMName()
    nic = createNic(vmName, location, 0)
    if not nic:
        return jsonify({})
    vmParameters = createVMParameters(vmName, nic.id, vm_size, location)
    async_vm_creation = compute_client.virtual_machines.create_or_update(
        os.environ.get('VM_GROUP'), vmParameters['vmName'], vmParameters['params'])
    async_vm_creation.wait()

    extension_parameters = {
        'location': location,
        'publisher': 'Microsoft.HpcCompute',
        'vm_extension_name': 'NvidiaGpuDriverWindows',
        'virtual_machine_extension_type': 'NvidiaGpuDriverWindows',
        'type_handler_version': '1.2'
    }

    async_vm_powershell = compute_client.virtual_machine_extensions.create_or_update(os.environ.get('VM_GROUP'),
                                                                                     vmParameters['vmName'], 'NvidiaGpuDriverWindows', extension_parameters)
    async_vm_powershell.wait()

    async_vm_start = compute_client.virtual_machines.start(
        os.environ.get('VM_GROUP'), vmParameters['vmName'])
    async_vm_start.wait()

    with open('app/scripts/vmCreate.txt', 'r') as file:
        command = file.read()
        run_command_parameters = {
            'command_id': 'RunPowerShellScript',
            'script': [
                command
            ]
        }
        poller = compute_client.virtual_machines.run_command(
            os.environ.get('VM_GROUP'),
            vmParameters['vmName'],
            run_command_parameters
        )
        result = poller.result()
        print(result.value[0].message)

    vm = getVM(vmParameters['vmName'])
    vm_ip = getIP(vm)
    updateVMIP(vmParameters['vmName'], vm_ip)

    return fetchVMCredentials(vmParameters['vmName'])


@celery.task(bind=True)
def createDisk(self, disk_size, username, location):
    _, compute_client, _ = createClients()
    disk_name = genDiskName()

    # Create managed data disk
    print('\nCreating (empty) managed Disk: ' + disk_name)
    async_disk_creation = compute_client.disks.create_or_update(
        os.environ.get('VM_GROUP'),
        disk_name,
        {
            'location': location,
            'disk_size_gb': disk_size,
            'creation_data': {
                'create_option': DiskCreateOption.empty
            }
        }
    )
    data_disk = async_disk_creation.result()

    vm_name = ""
    createDiskEntry(disk_name, vm_name, username, location)
    print("Disk created")
    attachDisk()

    return disk_name


@celery.task(bind=True)
def attachDisk(self, vm_name, disk_name):
    print('\nAttaching Disk')
    _, compute_client, _ = createClients()
    data_disk = compute_client.disks.get(os.environ.get('VM_GROUP'), disk_name)
    lunNum = 1
    attachedDisk = False
    while not attachedDisk:
        try:
            # Get the virtual machine by name
            print('Incrementing lun')
            virtual_machine = compute_client.virtual_machines.get(
                os.environ.get('VM_GROUP'),
                vm_name
            )
            virtual_machine.storage_profile.data_disks.append({
                'lun': lunNum,
                'name': disk_name,
                'create_option': DiskCreateOption.attach,
                'managed_disk': {
                    'id': data_disk.id
                }
            })
            async_disk_attach = compute_client.virtual_machines.create_or_update(
                os.environ.get('VM_GROUP'),
                virtual_machine.name,
                virtual_machine
            )
            attachedDisk = True
        except ClientException:
            lunNum += 1

    async_disk_attach.wait()

    command = '''
        Get-Disk | Where partitionstyle -eq 'raw' |
            Initialize-Disk -PartitionStyle MBR -PassThru |
            New-Partition -AssignDriveLetter -UseMaximumSize |
            Format-Volume -FileSystem NTFS -NewFileSystemLabel "{disk_name}" -Confirm:$false
        '''.format(disk_name=disk_name)

    run_command_parameters = {
        'command_id': 'RunPowerShellScript',
        'script': [
            command
        ]
    }
    poller = compute_client.virtual_machines.run_command(
        os.environ.get('VM_GROUP'),
        vm_name,
        run_command_parameters
    )

    result = poller.result()
    print("Disk attached to LUN#" + str(lunNum))
    print(result.value[0].message)


@celery.task(bind=True)
def detachDisk(self, vm_Name, disk_name):
    _, compute_client, _ = createClients()
    # Detach data disk
    print('\nDetach Disk: ' + disk_name)

    virtual_machine = compute_client.virtual_machines.get(
        os.environ.get('VM_GROUP'),
        vm_Name
    )
    data_disks = virtual_machine.storage_profile.data_disks
    data_disks[:] = [
        disk for disk in data_disks if disk.name != disk_name]
    async_vm_update = compute_client.virtual_machines.create_or_update(
        os.environ.get('VM_GROUP'),
        vm_Name,
        virtual_machine
    )
    virtual_machine = async_vm_update.result()
    print("Detach disk sucessful")


@celery.task(bind=True)
def fetchAll(self, update):
    _, compute_client, _ = createClients()
    vms = {'value': []}
    azure_portal_vms = compute_client.virtual_machines.list(
        os.getenv('VM_GROUP'))
    vm_usernames = []
    vm_names = []
    current_vms = {}

    if update:
        current_vms = fetchUserVMs(None)

    for entry in azure_portal_vms:
        vm_info = {}

        try:
            if current_vms[entry.name]['ip']:
                vm_info = {
                    'vm_name': entry.name,
                    'username': current_vms[entry.name]['username'],
                    'ip': current_vms[entry.name]['ip'],
                    'location': entry.location
                }
            else:
                vm = getVM(entry.name)
                vm_ip = getIP(vm)
                vm_info = {
                    'vm_name': entry.name,
                    'username': current_vms[entry.name]['username'],
                    'ip': vm_ip,
                    'location': entry.location
                }
                updateVMIP(entry.name, vm_ip)
        except:
            vm = getVM(entry.name)
            vm_ip = getIP(vm)
            vm_info = {
                'vm_name': entry.name,
                'username': entry.os_profile.admin_username,
                'ip': vm_ip,
                'location': entry.location
            }

        vms['value'].append(vm_info)
        vm_usernames.append(entry.os_profile.admin_username)
        vm_names.append(entry.name)

        if update:
            try:
                if not entry.name in current_names:
                    insertRow(entry.os_profile.admin_username,
                              entry.name, current_usernames, current_names)
            except:
                pass

    if update:
        for vm_name, vm_username in current_vms.items():
            deleteRow(vm_username, vm_name, vm_usernames, vm_names)

    return vms


@celery.task(bind=True)
def deleteVMResources(self, name):
    status = 200 if deleteResource(name) else 404
    return {'status': status}


@celery.task(bind=True)
def restartVM(self, vm_name):
    _, compute_client, _ = createClients()

    async_vm_restart = compute_client.virtual_machines.restart(
        os.environ.get('VM_GROUP'), vm_name)
    async_vm_restart.wait()

    return {'status': 200}
