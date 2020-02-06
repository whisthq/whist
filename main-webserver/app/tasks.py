from app import *
from .helperFuncs import *

@celery.task(bind = True)
def createVM(self, vm_size):
    _, compute_client, _ = createClients()
    vmName = genVMName()
    print("VM NAME!!!")
    print(vmName)
    nic = createNic(vmName, 0)
    if not nic: 
    	return jsonify({})
    vmParameters = createVMParameters(vmName, nic.id, vm_size)
    async_vm_creation = compute_client.virtual_machines.create_or_update(
        os.environ.get('VM_GROUP'), vmParameters['vmName'], vmParameters['params'])
    async_vm_creation.wait()
    async_vm_start = compute_client.virtual_machines.start(
        os.environ.get('VM_GROUP'), vmParameters['vmName'])
    async_vm_start.wait()
    return fetchVMCredentials(vmParameters['vmName'])

@celery.task(bind = True)
def fetchAll(self, update):
    _, compute_client, _ = createClients()
    vms = {'value': []}
    azure_portal_vms = compute_client.virtual_machines.list(os.getenv('VM_GROUP'))
    vm_usernames = []
    vm_names = []
    if update:
        current_vms = fetchUserVMs(None)
    for entry in azure_portal_vms:
        vm = getVM(entry.name)
        vm_ip = getIP(vm)
        vm_info = {
            'vm_name': entry.name,
            'username': entry.os_profile.admin_username,
            'ip': vm_ip
        }
        vms['value'].append(vm_info)
        vm_usernames.append(entry.os_profile.admin_username)
        vm_names.append(entry.name)

    if update:
        for current_vm in current_vms:
            updateRow(current_vm['vm_username'], current_vm['vm_name'], vm_usernames, vm_names)
    return vms
