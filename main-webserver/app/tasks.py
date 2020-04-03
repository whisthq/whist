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

    body = request.get_json()
    _, compute_client, _ = createClients()

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
    start = time.perf_counter()
    _, compute_client, _ = createClients()
    vms = {'value': []}
    azure_portal_vms = compute_client.virtual_machines.list(os.getenv('VM_GROUP'))
    vm_usernames = []
    vm_names = []
    current_usernames = []
    current_names = []
    end = time.perf_counter()
    print("{} seconds to fetch from azure".format(str(end - start)))

    start = time.perf_counter()
    if update:
        current_vms = fetchUserVMs(None)
        current_usernames = [current_vm['vm_username'] for current_vm in current_vms]
        current_names = [current_vm['vm_name'] for current_vm in current_vms]

    end = time.perf_counter()
    print("{} seconds to fetch from database".format(str(end - start)))

    for entry in azure_portal_vms:
        start = time.perf_counter()
        vm = getVM(entry.name)
        end = time.perf_counter()
        print("{} seconds to get VM".format(str(end - start)))
        start = time.perf_counter()
        vm_ip = getIP(vm)
        end = time.perf_counter()
        print("{} seconds to get IP".format(str(end - start)))
        vm_info = {}
        start = time.perf_counter()
        try:
            vm_info = {
                'vm_name': entry.name,
                'username': current_usernames[current_names.index(entry.name)],
                'ip': vm_ip,
                'location': entry.location
            }
        except:
            vm_info = {
                'vm_name': entry.name,
                'username': entry.os_profile.admin_username,
                'ip': vm_ip,
                'location': entry.location
            }  
        end = time.perf_counter()
        print("{} seconds to update dict".format(str(end - start)))

        vms['value'].append(vm_info)
        vm_usernames.append(entry.os_profile.admin_username)
        vm_names.append(entry.name)

        if update:
            try:
                print("Inserting into database")
                if not entry.name in current_names:
                    insertRow(entry.os_profile.admin_username, entry.name, current_usernames, current_names)
            except:
                pass

    if update:
        for current_vm in current_vms:
            print(current_vm['vm_name'])
            deleteRow(current_vm['vm_username'], current_vm['vm_name'], vm_usernames, vm_names)

    return vms


@celery.task(bind = True)
def deleteVMResources(self, name):
    status = 200 if deleteResource(name) else 404
    return {'status': status}