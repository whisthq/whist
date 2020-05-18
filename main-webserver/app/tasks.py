from app import *
from msrest.exceptions import ClientException
from .logger import *
from .helpers.general import *
from .helpers.vms import *
from .helpers.logins import *
from .helpers.disks import *
from .helpers.s3 import *


@celery.task(bind=True)
def createVM(self, vm_size, location, operating_system):
	"""Creates a windows vm of size vm_size in Azure region location

	Args:
		vm_size (str): The size of the vm to create
		location (str): The Azure region

	Returns:
		dict: The dict representing the vm in the v_ms sql table
	"""
	print("TASK: Create VM added to Redis queue")

	_, compute_client, _ = createClients()
	vmName = genVMName()
	nic = createNic(vmName, location, 0)
	if not nic:
		return
	vmParameters = createVMParameters(vmName, nic.id, vm_size, location)
	async_vm_creation = compute_client.virtual_machines.create_or_update(
		os.environ['VM_GROUP'], vmParameters['vm_name'], vmParameters['params'])
	async_vm_creation.wait()

	time.sleep(10)

	async_vm_start = compute_client.virtual_machines.start(
		os.environ['VM_GROUP'], vmParameters['vm_name'])
	async_vm_start.wait()

	time.sleep(30)

	extension_parameters = {
		'location': location,
		'publisher': 'Microsoft.HpcCompute',
		'vm_extension_name': 'NvidiaGpuDriverWindows',
		'virtual_machine_extension_type': 'NvidiaGpuDriverWindows',
		'type_handler_version': '1.2'
	}

	async_vm_extension = compute_client.virtual_machine_extensions.create_or_update(os.environ['VM_GROUP'],
																			 vmParameters['vm_name'], 'NvidiaGpuDriverWindows', extension_parameters)
	async_vm_extension.wait()

	vm = getVM(vmParameters['vm_name'])
	vm_ip = getIP(vm)
	updateVMIP(vmParameters['vm_name'], vm_ip)
	updateVMState(vmParameters['vm_name'], 'RUNNING_AVAILABLE')
	updateVMLocation(vmParameters['vm_name'], location)

	print('SUCCESS: VM {} created and updated'.format(vmName))

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
	attachDisk()

	return disk_name


@celery.task(bind=True)
def createDiskFromImage(self, username, location, vm_size, operating_system):
	hr = 400
	payload = None

	while hr == 400:
		print('Creating {} disk for {}'.format(operating_system, username))
		payload = createDiskFromImageHelper(username, location, vm_size, operating_system)
		hr = payload['status']
		print('Disk created with status {}'.format(hr))

	print(payload)
	payload['location'] = location
	return payload


@celery.task(bind=True)
def attachDisk(self, vm_name, disk_name):
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
	vm_ip = None

	if update:
		current_vms = fetchUserVMs(None)

	for entry in azure_portal_vms:
		print('NOTIFICATION: Found VM {} in Azure portal'.format(entry.name))
		vm_info = {}

		try:
			if current_vms[entry.name]['ip']:
				vm_ip = current_vms[entry.name]['ip']
				vm_info = {
					'vm_name': entry.name,
					'username': current_vms[entry.name]['username'],
					'ip': vm_ip,
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
			print('NOTIFICATION: Inserting {} into database'.format(entry.name))
			insertVM(entry.name)

	if update:
		updateVMStates()
		if current_vms:
			for vm_name, vm_username in current_vms.items():
				deleteRow(vm_name, vm_names)

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
def restartVM(self, vm_name, ID=-1):
	locked = checkLock(vm_name)

	while locked:
		print('NOTIFICATION: VM {} is locked. Waiting to be unlocked'.format(vm_name))
		time.sleep(5)
		locked = checkLock(vm_name)

	lockVM(vm_name, True)

	_, compute_client, _ = createClients()

	fractalVMStart(vm_name, True)

	lockVM(vm_name, False)
	sendInfo(ID, 'VM {} restarted successfully'.format(vm_name))

	return {'status': 200}


@celery.task(bind=True)
def startVM(self, vm_name, ID=-1):
	locked = checkLock(vm_name)

	while locked:
		print('NOTIFICATION: VM {} is locked. Waiting to be unlocked'.format(vm_name))
		time.sleep(5)
		locked = checkLock(vm_name)

	lockVM(vm_name, True)

	_, compute_client, _ = createClients()

	fractalVMStart(vm_name)
	lockVM(vm_name, False)

	sendInfo(ID, 'VM {} started successfully'.format(vm_name))

	return {'status': 200}


@celery.task(bind=True)
def updateVMStates(self):
	_, compute_client, _ = createClients()
	vms = compute_client.virtual_machines.list(
		resource_group_name=os.environ.get('VM_GROUP'))

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

		username = fetchVMCredentials(vm.name)
		if username:
			username = username['username']
			most_recent_action = getMostRecentActivity(username)
			if not most_recent_action:
				available = True
			elif most_recent_action['action'] == 'logoff':
				available = True

		vm_info = compute_client.virtual_machines.get(
			os.environ.get('VM_GROUP'), vm.name)
		if len(vm_info.storage_profile.data_disks) == 0:
			available = True

		updateVMStateAutomatically(vm.name)

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

@celery.task(bind = True)
def swapDiskSync(self, disk_name, ID = -1):
	def swapDiskAndUpdate(s, disk_name, vm_name):
		# Pick a VM, attach it to disk
		hr = swapdisk_name(s, disk_name, vm_name)
		if hr > 0:
			updateDisk(disk_name, vm_name, location)
			associateVMWithDisk(vm_name, disk_name)
			sendInfo(ID, "Database updated with VM {}, disk {}".format(vm_name, disk_name))
			return 1
		else:
			return -1

	def updateOldDisk(vm_name):
		virtual_machine = getVM(vm_name)
		old_disk = virtual_machine.storage_profile.os_disk
		updateDisk(old_disk.name, '', None)

	sendInfo(ID, " Swap disk task for disk {} added to Redis queue".format(disk_name))

	# Get the 
	_, compute_client, _ = createClients()

	if not disk_name:
		return None

	os_disk = compute_client.disks.get(os.environ.get('VM_GROUP'), disk_name)
	username = mapDiskToUser(disk_name)
	vm_name = os_disk.managed_by

	location = os_disk.location
	vm_attached = True if vm_name else False

	if vm_attached:
		self.update_state(state='PENDING', meta={"msg": "Boot request received successfully. Preparing your cloud PC."})
		sendInfo(ID, " Azure says that disk {} belonging to {} is attached to {}".format(disk_name, username, vm_name))
	else:
		self.update_state(state='PENDING', meta={"msg": "Boot request received successfully. Fetching your cloud PC."})
		sendInfo(ID, " Azure says that disk {} belonging to {} is not attached to any VM".format(disk_name, username))

	# Update the database to reflect the disk attached to the VM currently
	if vm_attached:
		vm_name = vm_name.split('/')[-1]
		sendInfo(ID, "{}is attached to {}".format(username, vm_name))

		unlocked = False
		while not unlocked and vm_attached:
			if spinLock(vm_name, s = self) > 0:
				unlocked = True
				# Lock immediately
				lockVMAndUpdate(vm_name = vm_name, state = 'ATTACHING', lock = True, temporary_lock = None, 
					change_last_updated = True, verbose = False, ID = ID)
				lockVM(vm_name, True, username = username, disk_name = disk_name, ID = ID)

				# Update database with new disk name and VM state
				sendInfo(ID, " Disk {} belongs to user {} and is already attached to VM {}".format(disk_name, username, vm_name))
				updateDisk(disk_name, vm_name, location)

				self.update_state(state='PENDING', meta={"msg": "Database updated. Sending signal to boot your cloud PC."})

				sendInfo(ID, ' Database updated with {} and {}'.format(disk_name, vm_name))

				if fractalVMStart(vm_name, s = self) > 0:
					sendInfo(ID, ' VM {} is started and ready to use'.format(vm_name))
					self.update_state(state='PENDING', meta={"msg": "Cloud PC is ready to use."})
				else:
					sendError(ID, ' Could not start VM {}'.format(vm_name))
					self.update_state(state='FAILURE', meta={"msg": "Cloud PC could not be started. Please contact support."})

				vm_credentials = fetchVMCredentials(vm_name)
				
				lockVMAndUpdate(vm_name = vm_name, state = 'RUNNING_AVAILABLE', lock = False, temporary_lock = 1, 
					change_last_updated = True, verbose = False, ID = ID)

				return vm_credentials
			else:
				os_disk = compute_client.disks.get(os.environ.get('VM_GROUP'), disk_name)
				vm_name = os_disk.managed_by
				location = os_disk.location
				vm_attached = True if vm_name else False
	
	if not vm_attached:
		disk_attached = False
		while not disk_attached: 
			vm = claimAvailableVM(disk_name, location, s = self)
			if vm:
				try:
					vm_name = vm['vm_name']
					sendInfo(ID, 'Disk {} was unattached. VM {} claimed for {}'.format(disk_name, vm_name, username))

					if swapDiskAndUpdate(self, disk_name, vm_name) > 0:
						self.update_state(state='PENDING', meta={"msg": "Data successfully uploaded to cloud PC."})
						free_vm_found = True
						updateOldDisk(vm_name)
						lockVM(vm_name, False)
						updateVMState(vm_name, 'RUNNING_AVAILABLE')
						return fetchVMCredentials(vm_name)

					lockVMAndUpdate(vm_name = vm_name, state = 'RUNNING_AVAILABLE', lock = False, temporary_lock = 1, 
						change_last_updated = True, verbose = False, ID = ID)

					disk_attached = True
					sendInfo(ID, ' VM {} successfully attached to disk {}'.format(vm_name, disk_name))

					vm_credentials = fetchVMCredentials(vm_name)
					return vm_credentials
				except Exception as e:
					sendCritical(ID, str(e))

			else:
				self.update_state(state='PENDING', meta={"msg": "Running performance tests. This could take a few extra minutes."})
				sendInfo(ID, 'No VMs are available for {} using {}. Going to sleep...'.format(username, disk_name))
				time.sleep(30)

	self.update_state(state='FAILURE', meta={"msg": "Cloud PC could not be started. Please contact support."})
	return None


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
	# print(f"SUCCESS: Disk swapped out in {end - start:0.4f} seconds. Restarting " + vm_name)

	start = time.perf_counter()
	fractalVMStart(vm_name)
	end = time.perf_counter()

	# print(f"SUCCESS: VM restarted in {end - start:0.4f} seconds")

	updateDisk(disk_name, vm_name, None)
	associateVMWithDisk(vm_name, disk_name)
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

	print('SUCCESS: {} deleted'.format(disk_name))

	return {'status': 200}


@celery.task(bind=True)
def deallocateVM(self, vm_name, ID=-1):
	lockVM(vm_name, True)

	_, compute_client, _ = createClients()

	sendInfo(ID, 'Starting to deallocate VM {}'.format(vm_name))
	updateVMState(vm_name, 'DEALLOCATING')

	async_vm_deallocate = compute_client.virtual_machines.deallocate(
		os.environ.get('VM_GROUP'), vm_name)
	async_vm_deallocate.wait()

	lockVM(vm_name, False)

	updateVMState(vm_name, 'DEALLOCATED')
	sendInfo(ID, 'VM {} deallocated successfully'.format(vm_name))

	return {'status': 200}


@celery.task(bind=True)
def storeLogs(self, sender, connection_id, logs, vm_ip, version, ID=-1):
	if SendLogsToS3(logs, sender, connection_id, vm_ip, version, ID) > 0:
		return {'status': 200}
	return {'status': 422}


@celery.task(bind=True)
def fetchLogs(self, username, fetch_all=False, ID=-1):
	if not fetch_all:
		sendInfo(ID, 'Fetch log for {} sent to Redis queue'.format(username))
		command = text("""
			SELECT * FROM logs WHERE "username" = :username ORDER BY last_updated DESC
			""")

		params = {'username': username}

		with engine.connect() as conn:
			sendInfo(ID, 'Fetching logs for {} from Postgres'.format(username))
			logs = cleanFetchedSQL(conn.execute(command, **params).fetchall())
			sendInfo(ID, 'Logs fetched for {} successfully'.format(username))
			conn.close()
			return logs
	else:
		sendInfo(ID, 'Fetch all logs sent to Redis queue')
		command = text("""
			SELECT * FROM logs ORDER BY last_updated DESC
			""")

		params = {'username': username}

		with engine.connect() as conn:
			sendInfo(ID, 'Fetching all logs from Postgres'.format(username))
			logs = cleanFetchedSQL(conn.execute(command, **params).fetchall())
			sendInfo(ID, 'All logs fetched successfully'.format(username))
			conn.close()
			return logs


@celery.task(bind=True)
def deleteLogs(self, connection_id, ID=-1):
	if deleteLogsInS3(connection_id, ID) > 0:
		return {'status': 200}
	return {'status': 422}
