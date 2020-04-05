from app import *
from .helperFuncs import *

@celery.task(bind = True)
def createVM(self, vm_size, location):
    _, compute_client, _ = createClients()
    vmName = genVMName()
    nic = createNic(vmName, location, 0)
    if not nic: 
    	return jsonify({})
    vmParameters = createVMParameters(vmName, nic.id, vm_size, location)
    async_vm_creation = compute_client.virtual_machines.create_or_update(
        os.environ.get('VM_GROUP'), vmParameters['vmName'], vmParameters['params'])

    extension_parameters= {
        'location': location,
        'publisher':'Microsoft.HpcCompute',
        'vm_extension_name':'NvidiaGpuDriverWindows',
        'virtual_machine_extension_type':'NvidiaGpuDriverWindows',
        'type_handler_version':'1.2'
    }
   
    compute_client.virtual_machine_extensions.create_or_update(os.environ.get('VM_GROUP'), 
        vmParameters['vmName'], 'NvidiaGpuDriverWindows', extension_parameters)

    async_vm_creation.wait()
    async_vm_start = compute_client.virtual_machines.start(
        os.environ.get('VM_GROUP'), vmParameters['vmName'])
    async_vm_start.wait()

    with open('app/powershell.txt', 'r') as file:
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

    return fetchVMCredentials(vmParameters['vmName'])

@celery.task(bind = True)
def fetchAll(self, update):
    _, compute_client, _ = createClients()
    vms = {'value': []}
    azure_portal_vms = compute_client.virtual_machines.list(os.getenv('VM_GROUP'))
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
                    insertRow(entry.os_profile.admin_username, entry.name, current_usernames, current_names)
            except:
                pass

    if update:
        for vm_name, vm_username in current_vms.items():
            deleteRow(vm_username, vm_name, vm_usernames, vm_names)

    return vms


@celery.task(bind = True)
def deleteVMResources(self, name):
    status = 200 if deleteResource(name) else 404
    return {'status': status}