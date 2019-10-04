from app import *
from .helperFuncs import *

@celery.task(bind = True)
def createVM(self, vm_size):
    _, compute_client, _ = createClients()
    vmName = genVMName()
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
