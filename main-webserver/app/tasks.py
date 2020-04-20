from app import *
from .helperFuncs import *
from msrest.exceptions import ClientException


@celery.task(bind=True)
def createVM(self, vm_size, location):
	print("TASK: Create VM added to Redis queue")

	_, compute_client, _ = createClients()
	vmName = genVMName()
	nic = createNic(vmName, location, 0)
	if not nic:
		return jsonify({})
	vmParameters = createVMParameters(vmName, nic.id, vm_size, location)
	async_vm_creation = compute_client.virtual_machines.create_or_update(
		os.environ.get('VM_GROUP'), vmParameters['vm_name'], vmParameters['params'])
	async_vm_creation.wait()

	extension_parameters = {
		'location': location,
		'publisher': 'Microsoft.HpcCompute',
		'vm_extension_name': 'NvidiaGpuDriverWindows',
		'virtual_machine_extension_type': 'NvidiaGpuDriverWindows',
		'type_handler_version': '1.2'
	}

	async_vm_powershell = compute_client.virtual_machine_extensions.create_or_update(os.environ.get('VM_GROUP'),
							vmParameters['vm_name'], 'NvidiaGpuDriverWindows', extension_parameters)
	async_vm_powershell.wait()

	async_vm_start = compute_client.virtual_machines.start(
		os.environ.get('VM_GROUP'), vmParameters['vm_name'])
	async_vm_start.wait()

	with open('app/scripts/vmCreate.txt', 'r') as file:
		print("TASK: Starting to run Powershell scripts")
		command = file.read()
		run_command_parameters = {
			'command_id': 'RunPowerShellScript',
			'script': [
				command
			]
		}

		poller = compute_client.virtual_machines.run_command(
			os.environ.get('VM_GROUP'),
			vmParameters['vm_name'],
			run_command_parameters
		)

		result = poller.result()
		print("SUCCESS: Powershell scripts finished running")
		print(result.value[0].message)

	vm = getVM(vmParameters['vm_name'])
	vm_ip = getIP(vm)
	updateVMIP(vmParameters['vm_name'], vm_ip)
	updateVMState(vmParameters['vm_name'], 'RUNNING_AVAILABLE')
	updateVMLocation(vmParameters['vm_name'], location)


	return fetchVMCredentials(vmParameters['vm_name'])


@celery.task(bind=True)
def createEmptyDisk(self, disk_size, username, location):
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
def createDiskFromImage(self, username, location, vm_size):
	hr = 400
	disk_name = None

	while hr == 400:
		payload = createDiskFromImageHelper(username, location, vm_size)
		hr = payload['status']
		disk_name = payload['disk_name']

	return {'disk_name': disk_name, 'location': location}


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
	print('\nDetach Data Disk: ' + disk_name)

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
	print("Detach data disk sucessful")


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
				if not entry.name in vm_names:
					insertRow(entry.os_profile.admin_username,
							  entry.name, current_usernames, vm_names)
			except:
				pass

	if update:
		for vm_name, vm_username in current_vms.items():
			deleteRow(vm_username, vm_name, vm_usernames, vm_names)

	return vms


@celery.task(bind=True)
def deleteVMResources(self, vm_name, delete_disk):
	locked = checkLock(vm_name)

	while locked:
		print('NOTIFICATION: VM {} is locked. Waiting to be unlocked'.format(vm_name))
		time.sleep(5)
		locked = checkLock(vm_name)
		
	lockVM(vm_name, True)

	status = 200 if deleteResource(vm_name, delete_disk) else 404

	lockVM(vm_name, False)
	return {'status': status}


@celery.task(bind=True)
def restartVM(self, vm_name):
	locked = checkLock(vm_name)

	while locked:
		print('NOTIFICATION: VM {} is locked. Waiting to be unlocked'.format(vm_name))
		time.sleep(5)
		locked = checkLock(vm_name)

	lockVM(vm_name, True)

	_, compute_client, _ = createClients()

	async_vm_restart = compute_client.virtual_machines.restart(
		os.environ.get('VM_GROUP'), vm_name)
	async_vm_restart.wait()

	lockVM(vm_name, False)
	return {'status': 200}

@celery.task(bind=True)
def startVM(self, vm_name):
	locked = checkLock(vm_name)

	while locked:
		print('NOTIFICATION: VM {} is locked. Waiting to be unlocked'.format(vm_name))
		time.sleep(5)
		locked = checkLock(vm_name)

	lockVM(vm_name, True)

	_, compute_client, _ = createClients()
	fractalVMStart(vm_name)

	lockVM(vm_name, False)
	return {'status': 200}


@celery.task(bind=True)
def updateVMStates(self):
	_, compute_client, _ = createClients()
	vms = compute_client.virtual_machines.list(
		resource_group_name = os.environ.get('VM_GROUP'))

	# looping inside the list of virtual machines, to grab the state of each machine
	for vm in vms:
		state = ''
		is_running = False
		available = False
		vm_state = compute_client.virtual_machines.instance_view(
			resource_group_name=os.environ.get('VM_GROUP'),
			vm_name=vm.name)
		if 'running' in vm_state.statuses[1].code:
			is_running = True

		username = fetchVMCredentials(vm.name)['username']
		if username:
			most_recent_action = getMostRecentActivity(username)
			if not most_recent_action:
				available = True
			elif most_recent_action['action'] == 'logoff':
				available = True

		vm_info = compute_client.virtual_machines.get(
			os.environ.get('VM_GROUP'), vm.name)
		if len(vm_info.storage_profile.data_disks) == 0:
			available = True

		if is_running and available:
			state = 'RUNNING_AVAILABLE'
		elif is_running and not available:
			state = 'RUNNING_UNAVAILABLE'
		elif not is_running and available:
			state = 'NOT_RUNNING_AVAILABLE'
		else:
			state = 'NOT_RUNNING_UNAVAILABLE'

		updateVMState(vm.name, state)

	return {'status': 200}



@celery.task(bind=True)
def syncDisks(self):
	_, compute_client, _ = createClients()
	disks = compute_client.disks.list(
		resource_group_name=os.environ.get('VM_GROUP'))
	disk_names = []

	for disk in disks:
		disk_name = disk.name
		disk_names.append(disk_name)
		disk_state = disk.disk_state
		vm_name = disk.managed_by
		if vm_name:
			vm_name = vm_name.split('/')[-1]
		else:
			vm_name = ''
		location = disk.location

		updateDisk(disk_name, vm_name, location)

	stored_disks = fetchUserDisks(None)
	for stored_disk in stored_disks:
		if not stored_disk['disk_name'] in disk_names:
			deleteDiskFromTable(stored_disk['disk_name'])

	return {'status': 200}


@celery.task(bind=True)
def swapDisk(self, disk_name):
	print("Swap disk task added to Redis queue")
	_, compute_client, _ = createClients()
	os_disk = compute_client.disks.get(os.environ.get('VM_GROUP'), disk_name)
	vm_name = os_disk.managed_by
	vm_attached = True if vm_name else False
	location = os_disk.location

	def swapDiskAndUpdate(disk_name, vm_name):
		# Pick a VM, attach it to disk
		hr = swapdisk_name(disk_name, vm_name)
		if hr > 0:
			updateDisk(disk_name, vm_name, location)
			associateVMWithDisk(vm_name, disk_name)
			updateVMState(vm_name, 'RUNNING_UNAVAILABLE')
			print("Database updated.")
			return 1
		else:
			return -1

	def updateOldDisk(vm_name):
		virtual_machine = getVM(vm_name)
		old_disk = virtual_machine.storage_profile.os_disk
		updateDisk(old_disk.name, '', None)
	# Disk is currently attached to a VM. Make sure the database reflects the current disk state,
	# and restart the VM as a sanity check.
	if vm_attached:
		vm_name = vm_name.split('/')[-1]
		print("NOTIFICATION: Disk " + disk_name +  " already attached to VM " + vm_name)

		locked = checkLock(vm_name)

		while locked:
			print('NOTIFICATION: VM {} is locked. Waiting to be unlocked'.format(vm_name))
			time.sleep(5)
			locked = checkLock(vm_name)

		print('NOTIFICATION: VM {} is unlocked and ready for use'.format(vm_name))
		lockVM(vm_name, True)

		updateDisk(disk_name, vm_name, location)
		associateVMWithDisk(vm_name, disk_name)
		updateVMState(vm_name, 'RUNNING_UNAVAILABLE')
		
		print("NOTIFICATION: Database updated with disk " + disk_name + " and " + vm_name)

		# If the VM is powered off, start it
		vm_state = compute_client.virtual_machines.instance_view(
			resource_group_name = os.environ.get('VM_GROUP'), vm_name = vm_name)

		print(vm_state.statuses[1])

		if fractalVMStart(vm_name) > 0:
			print('SUCCESS: VM is started and ready to use')
		else:
			print('CRITICAL ERROR: Could not start VM {}'.format(vm_name))
			
		vm_credentials = fetchVMCredentials(vm_name)
		lockVM(vm_name, False)
		return vm_credentials
	# Disk is currently in an unattached state. Find an available VM and attach the disk to that VM
	# (then reboot the VM), or wait until a VM becomes available.
	if not vm_attached or original_vm_name:
		free_vm_found = False
		while not free_vm_found:
			print("No VM attached to " + disk_name)
			available_vms = fetchAttachableVMs('RUNNING_AVAILABLE', location)
			if available_vms:
				print('Found ' + str(len(available_vms)) + ' available VMs')
				# Pick a VM, attach it to disk
				vm_name = available_vms[0]['vm_name']
				lockVM(vm_name, True)
				print('Selected VM ' + vm_name +
					  ' to attach to disk ' + disk_name)
				if swapDiskAndUpdate(disk_name, vm_name) > 0:
					free_vm_found = True
					updateOldDisk(vm_name)
					lockVM(vm_name, False)
					return fetchVMCredentials(vm_name)
				lockVM(vm_name, False)
				return {'status': 400}
			else:
				# Look for VMs that are not running
				print(
					"NOTIFICATION: Could not find a running and available VM to attach to disk " + disk_name)
				deactivated_vms = fetchAttachableVMs(
					'NOT_RUNNING_AVAILABLE', location)
				if deactivated_vms:
					vm_name = deactivated_vms[0]['vm_name']
					lockVM(vm_name, True)
					print("NOTIFICATION: Found deactivated VM " + vm_name)
					if swapDiskAndUpdate(disk_name, vm_name) > 0:
						free_vm_found = True
						updateOldDisk(vm_name)
						lockVM(vm_name, False)
						return fetchVMCredentials(vm_name)
					lockVM(vm_name, False)
					return {'status': 400}
				else:
					print("NOTIFICATION: No VMs are available. Going to sleep...")
					time.sleep(30)
	return {'status': 200}


@celery.task(bind=True)
def swapSpecificDisk(self, disk_name, vm_name):
	locked = checkLock(vm_name)

	while locked:
		print('NOTIFICATION: VM {} is locked. Waiting to be unlocked'.format(vm_name))
		time.sleep(5)
		locked = checkLock(vm_name)

	lockVM(vm_name, True)

	_, compute_client, _ = createClients()
	new_os_disk = compute_client.disks.get(
		os.environ.get('VM_GROUP'), disk_name)

	vm = getVM(vm_name)
	vm.storage_profile.os_disk.managed_disk.id = new_os_disk.id
	vm.storage_profile.os_disk.name = new_os_disk.name

	print('Swapping out disk ' + disk_name + ' on VM ' + vm_name)
	start = time.perf_counter()

	async_disk_attach = compute_client.virtual_machines.create_or_update(
		'Fractal', vm.name, vm
	)
	async_disk_attach.wait()

	end = time.perf_counter()
	print(f"SUCCESS: Disk swapped out in {end - start:0.4f} seconds. Restarting " + vm_name)

	start = time.perf_counter()
	async_vm_restart = compute_client.virtual_machines.restart(
		'Fractal', vm.name)
	async_vm_restart.wait()
	end = time.perf_counter()

	print(f"SUCCESS: VM restarted in {end - start:0.4f} seconds")


	updateDisk(disk_name, vm_name, None)
	associateVMWithDisk(vm_name, disk_name)
	updateVMState(vm_name, 'RUNNING_UNAVAILABLE')
	print("Database updated.")

	time.sleep(10)

	print('VM ' + vm_name + ' successfully restarted')

	lockVM(vm_name, False)
	return fetchVMCredentials(vm_name)

@celery.task(bind=True)
def updateVMTable(self):
	vms = fetchUserVMs(None)
	_, compute_client, network_client = createClients()
	azure_portal_vms = [entry.name for entry in compute_client.virtual_machines.list(
		os.getenv('VM_GROUP'))]

	for vm in vms:
		try:
			if not vm['vm_name'] in azure_portal_vms:
				deleteVMFromTable(vm['vm_name'])
			else:
				vm_name = vm['vm_name']
				vm = getVM(vm_name)
				os_disk_name = vm.storage_profile.os_disk.name
				username = mapDiskToUser(os_disk_name)
				updateVM(vm_name, vm.location, os_disk_name, username)
		except Exception as e:
			print("ERROR: " + e)
			pass

	return {'status': 200}

@celery.task(bind=True)
def runPowershell(self, vm_name):
	_, compute_client, _ = createClients()
	with open('app/scripts/vmCreate.txt', 'r') as file:
		print("TASK: Starting to run Powershell scripts")
		command = file.read()
		run_command_parameters = {
			'command_id': 'RunPowerShellScript',
			'script': [command]
		}

		poller = compute_client.virtual_machines.run_command(
			os.environ.get('VM_GROUP'),
			vm_name,
			run_command_parameters
		)
		# poller.wait()
		result = poller.result()
		print("SUCCESS: Powershell scripts finished running")
		print(result.value[0].message)

	return {'status': 200}

@celery.task(bind=True)
def deleteDisk(self, disk_name):
	_, compute_client, _ = createClients()
	try:
		print("Attempting to delete the OS disk...")
		os_disk_delete = compute_client.disks.delete(
			os.getenv('VM_GROUP'), disk_name)
		os_disk_delete.wait()
		deleteDiskFromTable(disk_name)
		print("OS disk deleted")
	except Exception as e:
		print('ERROR: ' + str(e))
		updateDiskState(disk_name, 'TO_BE_DELETED')
		
		return {'status': 200}

@celery.task(bind=True)
def deallocateVM(self, vm_name):
	lockVM(vm_name, True)

	_, compute_client, _ = createClients()

	async_vm_deallocate = compute_client.virtual_machines.deallocate(
		os.environ.get('VM_GROUP'), vm_name)
	async_vm_deallocate.wait()

	lockVM(vm_name, False)
	return {'status': 200}
