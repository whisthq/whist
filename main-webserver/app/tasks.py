from app import *
from .helperFuncs import *

@celery.task(bind = True)
def make_file(self):
    

@celery.task(bind = True)
def createVM(self):
    _, compute_client, _ = createClients()
    nic = createNic()
    vmParameters = createVMParameters(nic.id)
    async_vm_creation = compute_client.virtual_machines.create_or_update(
        os.environ.get('VM_GROUP'), vmParameters['vmName'], vmParameters['params'])
    async_vm_creation.wait()
    async_vm_start = compute_client.virtual_machines.start(
        os.environ.get('VM_GROUP'), vmParameters['vmName'])
    async_vm_start.wait()
    return vmParameters['vmName'], \
    	vmParameters['params']['os_profile']['admin_username'], \
    	vmParameters['params']['os_profile']['admin_password'], \
    	vmParameters['params']['network_profile']['network_interfaces'][0]['id']
