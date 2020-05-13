from .utils import *
from app import *
from .logger import *


def createClients():
    """Creates Azure management clients

    This function is used to access the resource management, compute management, and network management clients, which are used to access the Azure VM API

    Returns:
    ResourceManagementClient, ComputeManagementClient, NetworkManagementClient

   """
    subscription_id = os.getenv('AZURE_SUBSCRIPTION_ID')
    credentials = ServicePrincipalCredentials(
        client_id=os.getenv('AZURE_CLIENT_ID'),
        secret=os.getenv('AZURE_CLIENT_SECRET'),
        tenant=os.getenv('AZURE_TENANT_ID')
    )
    r = ResourceManagementClient(credentials, subscription_id)
    c = ComputeManagementClient(credentials, subscription_id)
    n = NetworkManagementClient(credentials, subscription_id)
    return r, c, n


def createNic(name, location, tries):
    _, _, network_client = createClients()
    vnetName, subnetName, ipName, nicName = name + \
        '_vnet', name + '_subnet', name + '_ip', name + '_nic'
    try:
        async_vnet_creation = network_client.virtual_networks.create_or_update(
            os.getenv('VM_GROUP'),
            vnetName,
            {
                'location': location,
                'address_space': {
                    'address_prefixes': ['10.0.0.0/16']
                }
            }
        )
        async_vnet_creation.wait()

        # Create Subnet
        async_subnet_creation = network_client.subnets.create_or_update(
            os.getenv('VM_GROUP'),
            vnetName,
            subnetName,
            {'address_prefix': '10.0.0.0/24'}
        )
        subnet_info = async_subnet_creation.result()

        # Create public IP address
        public_ip_addess_params = {
            'location': location,
            'public_ip_allocation_method': 'Static'
        }
        creation_result = network_client.public_ip_addresses.create_or_update(
            os.getenv('VM_GROUP'),
            ipName,
            public_ip_addess_params
        )

        public_ip_address = network_client.public_ip_addresses.get(
            os.getenv('VM_GROUP'),
            ipName)

        # Create NIC
        async_nic_creation = network_client.network_interfaces.create_or_update(
            os.getenv('VM_GROUP'),
            nicName,
            {
                'location': location,
                'ip_configurations': [{
                    'name': ipName,
                    'public_ip_address': public_ip_address,
                    'subnet': {
                        'id': subnet_info.id
                    }
                }]
            }
        )

        return async_nic_creation.result()
    except Exception as e:
        if tries < 5:
            print(e)
            time.sleep(3)
            return createNic(name, location, tries + 1)
        else:
            return None


def deleteResource(name, delete_disk, ID=-1):
    """Deletes a vm

    Parameters:
    name (str): the name of the VM
    delete_disk (bool): whether or not to delete the disk attached to the vm
    ID (int): a unique identifier for the method in PaperTrail (default is -1)

    Returns:
    int: Error flag: 1 success, -1 fail

   """
    _, compute_client, network_client = createClients()
    vnetName, subnetName, ipName, nicName = name + \
        '_vnet', name + '_subnet', name + '_ip', name + '_nic'
    hr = 1

    # get VM info based on name
    virtual_machine = getVM(name)
    os_disk_name = virtual_machine.storage_profile.os_disk.name

    # step 1, deallocate the VM
    try:
        sendInfo(ID, 'Attempting to deallocate the VM {}'.format(name))
        async_vm_deallocate = compute_client.virtual_machines.deallocate(
            os.getenv('VM_GROUP'), name)
        async_vm_deallocate.wait()
        # wait a whole minute to ensure it deallocated properly
        time.sleep(60)
        sendInfo(ID, 'VM {} deallocated successfully'.format(name))
    except Exception as e:
        sendError(ID, str(e))
        hr = -1

    # step 2, detach the IP
    try:
        sendInfo(ID, "Attempting to detach the IP of VM {}".format(name))
        subnet_obj = network_client.subnets.get(
            resource_group_name=os.getenv('VM_GROUP'),
            virtual_network_name=vnetName,
            subnet_name=subnetName)
        # configure network interface parameters.
        params = {
            'location': virtual_machine.location,
            'ip_configurations': [{
                'name': ipName,
                'subnet': {'id': subnet_obj.id},
                # None: Disassociate;
                'public_ip_address': None,
            }]
        }
        # use method create_or_update to update network interface configuration.
        async_ip_detach = network_client.network_interfaces.create_or_update(
            resource_group_name=os.getenv('VM_GROUP'),
            network_interface_name=nicName,
            parameters=params)
        async_ip_detach.wait()
        sendInfo(ID, 'IP detached from VM {} successfully'.format(name))
    except Exception as e:
        sendError(ID, str(e))
        hr = -1

    # step 3, delete the VM
    try:
        sendInfo(ID, "Attempting to delete VM {}".format(name))
        async_vm_delete = compute_client.virtual_machines.delete(
            os.getenv('VM_GROUP'), name)
        async_vm_delete.wait()
        sendInfo(ID, 'VM {} deleted successfully'.format(name))
        deleteVMFromTable(name)
        sendInfo(ID, 'VM {} removed from Postgres'.format(name))
    except Exception as e:
        sendError(ID, str(e))
        hr = -1

    # step 4, delete the IP
    try:
        sendInfo(ID, "Attempting to delete ID from VM {}".format(name))
        async_ip_delete = network_client.public_ip_addresses.delete(
            os.getenv('VM_GROUP'),
            ipName
        )
        async_ip_delete.wait()
        sendInfo(ID, 'IP deleted from VM {} successfully'.format(name))
    except Exception as e:
        sendError(ID, str(e))
        hr = -1

    # step 4, delete the NIC
    try:
        sendInfo(ID, "Attempting to delete NIC from VM {}".format(name))
        async_nic_delete = network_client.network_interfaces.delete(
            os.getenv('VM_GROUP'),
            nicName
        )
        async_nic_delete.wait()
        sendInfo(ID, 'NIC deleted from VM {} successfully'.format(name))
    except Exception as e:
        sendError(ID, str(e))
        hr = -1

    # step 5, delete the Vnet
    try:
        sendInfo(ID, "Attempting to delete vnet from VM {}".format(name))
        async_vnet_delete = network_client.virtual_networks.delete(
            os.getenv('VM_GROUP'),
            vnetName
        )
        async_vnet_delete.wait()
        sendInfo(ID, 'Vnet deleted from VM {} successfully'.format(name))
    except Exception as e:
        sendError(ID, str(e))
        hr = -1

    if delete_disk:
        # step 6, delete the OS disk -- not needed anymore (OS disk swapping)
        try:
            sendInfo(
                ID, "Attempting to delete OS disk from VM {}".format(name))
            os_disk_delete = compute_client.disks.delete(
                os.getenv('VM_GROUP'), os_disk_name)
            os_disk_delete.wait()
            sendInfo(ID, 'Disk {} deleted from VM {} successfully'.format(
                os_disk_name, name))
        except Exception as e:
            sendError(ID, str(e))
            hr = -1

    return hr


def createVMParameters(vmName, nic_id, vm_size, location, operating_system='Windows'):
    """Adds a vm entry to the SQL database

    Parameters:
    vmName (str): The name of the VM to add
    nic_id (str): The vm's network interface ID
    vm_size (str): The type of vm in terms of specs(default is NV6)
    location (str): The Azure region of the vm
    operating_system (str): The operating system of the vm (default is 'Windows')

    Returns:
    dict: Parameters that will be used in Azure sdk
   """

    with engine.connect() as conn:
        oldUserNames = [cell[0] for cell in list(
            conn.execute('SELECT "username" FROM v_ms'))]
        userName = genHaiku(1)[0]
        while userName in oldUserNames:
            userName = genHaiku(1)

        vm_reference = {
            'publisher': 'MicrosoftWindowsDesktop',
            'offer': 'Windows-10',
            'sku': 'rs5-pro',
            'version': 'latest'
        } if operating_system == 'Windows' else {
            "publisher": "Canonical",
            "offer": "UbuntuServer",
            "sku": "18.04-LTS",
            "version": "latest"
        }

        command = text("""
            INSERT INTO v_ms("vm_name", "disk_name")
            VALUES(:vmName, :disk_name)
            """)
        params = {'vmName': vmName,
                  'username': userName, 'disk_name': None}
        with engine.connect() as conn:
            conn.execute(command, **params)
            conn.close()

            return {'params': {
                'location': location,
                'os_profile': {
                    'computer_name': vmName,
                    'admin_username': os.getenv('VM_GROUP'),
                    'admin_password': os.getenv('VM_PASSWORD'),
                    'secrets': [
                        {
                            'sourceVault': {
                                'id': '497f0f14-93c3-46f4-b636-de61e2240a84'
                            },
                            'vaultCertificates': [
                                {
                                    'certificateUrl': 'https://fractalkeyvault.vault.azure.net/secrets/FractalWinRMSecret/2d88b71f863f4fa88102e1e6fff73522',
                                    'certificateStore': 'FractalWinRMSecret'
                                }
                            ]
                        }
                    ],
                    'windowsConfiguration': {
                        'provisionVMAgent': true,
                        'enableAutomaticUpdates': true,
                        'winRM': {
                            'listeners': [
                                {
                                    'protocol': 'http'
                                },
                                {
                                    'protocol': 'https',
                                    'certificateUrl': 'https://fractalkeyvault.vault.azure.net/secrets/FractalWinRMSecret/2d88b71f863f4fa88102e1e6fff73522'
                                }
                            ]
                        },
                    }
                },
                'hardware_profile': {
                    'vm_size': vm_size
                },
                'storage_profile': {
                    'image_reference': {
                        'publisher': vm_reference['publisher'],
                        'offer': vm_reference['offer'],
                        'sku': vm_reference['sku'],
                        'version': vm_reference['version']
                    },
                    'os_disk': {
                        'os_type': operating_system,
                        'create_option': 'FromImage'
                    }
                },
                'network_profile': {
                    'network_interfaces': [{
                        'id': nic_id,
                    }]
                },
            }, 'vm_name': vmName}


def createDiskEntry(disk_name, vm_name, username, location, state="ACTIVE"):
    """Adds a disk to the disks SQL database

    Parameters:
    disk_name(str): The name of the disk to add
    vmName (str): The name of the VM that it is attached to
    username (str): The username of the disk's respective user
    location (str): The Azure region of the vm
    state (str): The state of the disk (default is "ACTIVE")
   """
    with engine.connect() as conn:
        command = text("""
            INSERT INTO disks("disk_name", "vm_name", "username", "location", "state")
            VALUES(:diskname, :vmname, :username, :location, :state)
            """)
        params = {
            'diskname': disk_name,
            'vmname': vm_name,
            'username': username,
            "location": location,
            "state": state
        }
        with engine.connect() as conn:
            conn.execute(command, **params)
            conn.close()


def getVM(vm_name):
    """Retrieves information about the model view or the instance view of an Azure virtual machine

    Parameters:
    vmName (str): The name of the VM to retrieve

    Returns:
    VirtualMachine: The instance view of the virtual machine
   """
    _, compute_client, _ = createClients()
    try:
        virtual_machine = compute_client.virtual_machines.get(
            os.environ.get('VM_GROUP'),
            vm_name
        )
        return virtual_machine
    except:
        return None


def singleValueQuery(value):
    """Queries all vms from the v_ms SQL table with the specified name

    Parameters:
    vmName (str): The name of the VM to retrieve

    Returns:
    dict: The dictionary respresenting the query result
   """
    command = text("""
        SELECT * FROM v_ms WHERE "vm_name" = :value
        """)
    params = {'value': value}
    with engine.connect() as conn:
        exists = cleanFetchedSQL(
            conn.execute(command, **params).fetchall())
        conn.close()
        return True if exists else False


def getIP(vm):
    _, _, network_client = createClients()
    ni_reference = vm.network_profile.network_interfaces[0]
    ni_reference = ni_reference.id.split('/')
    ni_group = ni_reference[4]
    ni_name = ni_reference[8]

    net_interface = network_client.network_interfaces.get(
        ni_group, ni_name)
    ip_reference = net_interface.ip_configurations[0].public_ip_address
    ip_reference = ip_reference.id.split('/')
    ip_group = ip_reference[4]
    ip_name = ip_reference[8]

    public_ip = network_client.public_ip_addresses.get(ip_group, ip_name)
    return public_ip.ip_address


def updateVMUsername(username, vm_name):
    """Updates the username associated with a vm entry in the v_ms SQL table

    Parameters:
    username (str): The new username
    vm_name (str): The name of the VM row to update
   """
    command = text("""
        UPDATE v_ms
        SET username = :username
        WHERE
        "vm_name" = :vm_name
        """)
    params = {'username': username, 'vm_name': vm_name}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def loginUser(username, password):
    """Verifies the username password combination in the users SQL table

    If the password is the admin password, just check if the username exists
    Else, check to see if the username is in the database and the jwt encoded password is in the database

    Parameters:
    username (str): The username
    vm_name (str): The password

    Returns:
    bool: True if authentication success, False otherwise
   """

    if password != os.getenv('ADMIN_PASSWORD'):
        command = text("""
            SELECT * FROM users WHERE "username" = :userName AND "password" = :password
            """)
        pwd_token = jwt.encode({'pwd': password}, os.getenv('SECRET_KEY'))
        params = {'userName': username, 'password': pwd_token}
        with engine.connect() as conn:
            user = cleanFetchedSQL(conn.execute(
                command, **params).fetchall())
            conn.close()
            return True if user else False
    else:
        command = text("""
            SELECT * FROM users WHERE "username" = :userName
            """)
        params = {'userName': username}
        with engine.connect() as conn:
            user = cleanFetchedSQL(conn.execute(
                command, **params).fetchall())
            conn.close()
            return True if user else False


def lookup(username):
    """Looks up the username in the users SQL table

    Args:
        username (str): The username to look up

    Returns:
        bool: True if user exists, False otherwise
    """

    command = text("""
        SELECT * FROM users WHERE "username" = :userName
        """)
    params = {'userName': username}
    with engine.connect() as conn:
        user = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        conn.close()
        return True if user else False


def genUniqueCode():
    """Generates a unique referral code

    Returns:
        int: The generated code
    """
    with engine.connect() as conn:
        old_codes = [cell[0]
                     for cell in list(conn.execute('SELECT "code" FROM users'))]
        new_code = generateCode()
        while new_code in old_codes:
            new_code = generateCode()
        return new_code


def registerUser(username, password, token):
    """Registers a user, and stores it in the users table

    Args:
        username (str): The username
        password (str): The password (to be encoded into a jwt token)
        token (str): The email comfirmation token

    Returns:
        int: 200 on success, 400 on fail
    """
    pwd_token = jwt.encode({'pwd': password}, os.getenv('SECRET_KEY'))
    code = genUniqueCode()
    command = text("""
        INSERT INTO users("username", "password", "code", "id")
        VALUES(:userName, :password, :code, :token)
        """)
    params = {'userName': username, 'password': pwd_token,
              'code': code, 'token': token}
    with engine.connect() as conn:
        try:
            conn.execute(command, **params)
            conn.close()
            return 200
        except:
            return 400


def generateIDs():
    """Generates an email verification token for all users in the users SQL table
    """
    with engine.connect() as conn:
        for row in list(conn.execute('SELECT * FROM users')):
            token = generateToken(row[0])
            command = text("""
                UPDATE users
                SET "id" = :token
                WHERE "username" = :userName
                """)
            params = {'token': token, 'userName': row[0]}
            conn.execute(command, **params)
            conn.close()


def resetPassword(username, password):
    """Updates the password for a user in the users SQL table

    Args:
        username (str): The user to update the password for
        password (str): The new password
    """
    pwd_token = jwt.encode({'pwd': password}, os.getenv('SECRET_KEY'))
    command = text("""
        UPDATE users
        SET "password" = :password
        WHERE "username" = :userName
        """)
    params = {'userName': username, 'password': pwd_token}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def fetchVMCredentials(vm_name):
    """Fetches a vm from the v_ms sql table

    Args:
        vm_name (str): The name of the vm to fetch

    Returns:
        dict: An object respresenting the respective row in the table
    """
    command = text("""
        SELECT * FROM v_ms WHERE "vm_name" = :vm_name
        """)
    params = {'vm_name': vm_name}
    with engine.connect() as conn:
        vm_info = cleanFetchedSQL(
            conn.execute(command, **params).fetchone())
        conn.close()
        return vm_info


def fetchVMByIP(vm_ip):
    """Fetches a vm from the v_ms sql table

    Args:
        vm_ip (str): The ip address (ipv4) of the vm to fetch

    Returns:
        dict: An object respresenting the respective row in the table
    """
    command = text("""
        SELECT * FROM v_ms WHERE "ip" = :vm_ip
        """)
    params = {'vm_ip': vm_ip}
    with engine.connect() as conn:
        vm_info = cleanFetchedSQL(
            conn.execute(command, **params).fetchone())
        conn.close()
        return vm_info


def genVMName():
    """Generates a unique name for a vm

    Returns:
        str: The generated name
    """
    with engine.connect() as conn:
        oldVMs = [cell[0]
                  for cell in list(conn.execute('SELECT "vm_name" FROM v_ms'))]
        vmName = genHaiku(1)[0]
        while vmName in oldVMs:
            vmName = genHaiku(1)[0]
        return vmName


def genDiskName():
    """Generates a unique name for a disk

    Returns:
        str: The generated name
    """
    with engine.connect() as conn:
        oldDisks = [cell[0] for cell in list(
            conn.execute('SELECT "disk_name" FROM disks'))]
        diskName = genHaiku(1)[0]
        while diskName in oldDisks:
            diskName = genHaiku(1)[0]
        return str(diskName)


def addTimeTable(username, action, time, is_user, ID=-1):
    """Adds a user action to the login_history sql table

    Args:
        username (str): The username of the user
        action (str): ['logon', 'logoff']
        time (str): Time in the format mm-dd-yyyy, hh:mm:ss
        is_user (bool): Whether an actual user did the action, vs admin
        ID (int, optional): Papertrail loggind ID. Defaults to -1.
    """
    command = text("""
        INSERT INTO login_history("username", "timestamp", "action", "is_user")
        VALUES(:userName, :currentTime, :action, :is_user)
        """)

    aware = dt.now()
    now = aware.strftime('%m-%d-%Y, %H:%M:%S')

    sendInfo(ID, 'Adding to time table a {} at time {}'.format(action, now))

    params = {'userName': username, 'currentTime': now,
              'action': action, 'is_user': is_user}

    with engine.connect() as conn:
        conn.execute(command, **params)

        disk = fetchUserDisks(username)
        if disk:
            disk_name = disk[0]['disk_name']
            vms = mapDiskToVM(disk_name)
            if vms:
                vm_name = vms[0]['vm_name']

                if action == 'logoff':
                    lockVM(vm_name, False)
                elif action == 'logon':
                    lockVM(vm_name, True)
            else:
                sendCritical(
                    ID, 'Could not find a VM currently attached to disk {}'.format(disk_name))
        else:
            sendCritical(
                ID, 'Could not find a disk belong to user {}'.format(username))

        conn.close()


def fetchUserVMs(username):
    """Fetches vms that are connected to the user

    Args:
        username (str): The username

    Returns:
        dict: The object representing the respective vm in the v_ms sql database
    """
    if username:
        command = text("""
            SELECT * FROM v_ms WHERE "username" = :username
            """)
        params = {'username': username}
        with engine.connect() as conn:
            vms_info = cleanFetchedSQL(
                conn.execute(command, **params).fetchall())
            conn.close()
            return vms_info
    else:
        command = text("""
            SELECT * FROM v_ms
            """)
        params = {}
        with engine.connect() as conn:
            vms_info = cleanFetchedSQL(
                conn.execute(command, **params).fetchall())
            conn.close()
            return vms_info


def getVMSize(disk_name):
    """Gets the size of the vm

    Args:
        disk_name (str): Name of the disk attached to the vm

    Returns:
        str: The size of the disk attached to the vm
    """
    command = text("""
        SELECT * FROM disks WHERE "disk_name" = :disk_name
        """)
    params = {'disk_name': disk_name}
    with engine.connect() as conn:
        disks_info = cleanFetchedSQL(
            conn.execute(command, **params).fetchone())
        conn.close()
        return disks_info['vm_size']


def fetchUserDisks(username, show_all=False, ID=-1):
    """Fetches all disks associated with the user

    Args:
        username (str): The username. If username is null, it fetches all disks
        show_all (bool, optional): Whether or not to select all disks regardless of state, vs only disks with ACTIVE state. Defaults to False.
        ID (int, optional): Papertrail logging ID. Defaults to -1.

    Returns:
        array: An array of the disks
    """
    if username:
        if not show_all:
            sendInfo(
                ID, 'Fetching all disks associated with {} state ACTIVE'.format(username))

            command = text("""
                SELECT * FROM disks WHERE "username" = :username AND "state" = :state
                """)
            params = {'username': username, 'state': 'ACTIVE'}
            with engine.connect() as conn:
                sendInfo(ID, 'Connection with Postgres established')

                disks_info = cleanFetchedSQL(
                    conn.execute(command, **params).fetchall())
                conn.close()

                if disks_info:
                    sendInfo(
                        ID, 'Disk names fetched and Postgres connection closed')
                else:
                    sendWarning(
                        ID, 'No disk found for {}. Postgres connection closed')

                return disks_info
        else:
            sendInfo(
                ID, 'Fetching all disks associated with {} regardless of state'.format(username))

            command = text("""
                SELECT * FROM disks WHERE "username" = :username
                """)
            params = {'username': username}
            with engine.connect() as conn:
                sendInfo(ID, 'Connection with Postgres established')

                disks_info = cleanFetchedSQL(
                    conn.execute(command, **params).fetchall())
                conn.close()

                if disks_info:
                    sendInfo(
                        ID, 'Disk names fetched and Postgres connection closed')
                else:
                    sendWarning(
                        ID, 'No disk found for {}. Postgres connection closed')

                return disks_info
    else:
        sendInfo(ID, 'Fetching all disks in Postgres')

        command = text("""
            SELECT * FROM disks
            """)
        params = {}
        with engine.connect() as conn:
            sendInfo(ID, 'Connection with Postgres established')

            disks_info = cleanFetchedSQL(
                conn.execute(command, **params).fetchall())
            conn.close()

            if disks_info:
                sendInfo(
                    ID, 'Disk names fetched and Postgres connection closed')
            else:
                sendWarning(
                    ID, 'No disk found in Postgres. Postgres connection closed')

            return disks_info


def fetchUserCode(username):
    """Fetches the verification code of a user

    Args:
        username (str): The username of the user of interest

    Returns:
        str: The verification code
    """
    try:
        command = text("""
            SELECT * FROM users WHERE "username" = :userName
            """)
        params = {'userName': username}
        with engine.connect() as conn:
            user = cleanFetchedSQL(conn.execute(
                command, **params).fetchone())
            conn.close()
            return user['code']
    except:
        return None


def deleteRow(vm_name, vm_names):
    """Deletes a row from the v_ms sql table, if vm_name is not in vm_names

    Args:
        vm_name (str): The name of the vm to check
        vm_names (array[string]): An array of the names of accepted VMs
    """
    if not (vm_name in vm_names):
        command = text("""
            DELETE FROM v_ms WHERE "vm_name" = :vm_name
            """)
        params = {'vm_name': vm_name}
        with engine.connect() as conn:
            conn.execute(command, **params)
            conn.close()


def deleteUser(username):
    """Deletes a user from the users sql table

    Args:
        username (str): The name of the user

    Returns:
        int: 200 for successs, 404 for failure
    """
    command = text("""
        DELETE FROM users WHERE "username" = :username
        """)
    params = {'username': username}
    with engine.connect() as conn:
        try:
            conn.execute(command, **params)
            conn.close()
            return 200
        except:
            return 404


def insertVM(vm_name, vm_ip=None, location=None):
    """Inserts a vm into the v_ms table

    Args:
        vm_name (str): The name of the vm
        vm_ip (str, optional): The ipv4 address of the vm. Defaults to None.
        location (str, optional): The region that the vm is based in. Defaults to None.
    """
    command = text("""
        SELECT * FROM v_ms WHERE "vm_name" = :vm_name
        """)
    params = {'vm_name': vm_name}
    with engine.connect() as conn:
        vms = cleanFetchedSQL(conn.execute(command, **params).fetchall())

        if not vms:
            _, compute_client, _ = createClients()
            vm = getVM(vm_name)

            if not vm_ip or not location:
                vm_ip = getIP(vm)
                location = vm.location

            disk_name = vm.storage_profile.os_disk.name
            username = mapDiskToUser(disk_name)
            lock = False
            dev = False

            command = text("""
                INSERT INTO v_ms("vm_name", "username", "disk_name", "ip", "location", "lock", "dev")
                VALUES(:vm_name, :username, :disk_name, :ip, :location, :lock, :dev)
                """)
            params = {'username': username,
                      'vm_name': vm_name,
                      'disk_name': disk_name,
                      'ip': vm_ip,
                      'location': location,
                      'lock': False,
                      'dev': False}

            conn.execute(command, **params)
            conn.close()


def fetchLoginActivity():
    """Fetches the entire login_history sql table

    Returns:
        array[dict]: The login history
    """
    command = text("""
        SELECT * FROM login_history
        """)
    params = {}
    with engine.connect() as conn:
        activities = cleanFetchedSQL(
            conn.execute(command, **params).fetchall())
        activities.reverse()
        conn.close()
        return activities


def fetchCustomer(username):
    """Fetches the customer from the customers sql table by username

    Args:
        username (str): The customer name

    Returns:
        dict: A dictionary that represents the customer
    """
    command = text("""
        SELECT * FROM customers WHERE "username" = :username
        """)
    params = {'username': username}
    with engine.connect() as conn:
        customer = cleanFetchedSQL(
            conn.execute(command, **params).fetchone())
        conn.close()
        return customer


def fetchCustomerById(id):
    """Fetches the customer from the customers sql table by id

    Args:
        id (str): The unique id of the customer

    Returns:
        dict: A dictionary that represents the customer
    """
    command = text("""
        SELECT * FROM customers WHERE "id" = :id
        """)
    params = {'id': id}
    with engine.connect() as conn:
        customer = cleanFetchedSQL(
            conn.execute(command, **params).fetchone())
        conn.close()
        return customer


def fetchCustomers():
    """Fetches all customers from the customers sql table

    Returns:
        arr[dict]: An array of all the customers
    """
    command = text("""
        SELECT * FROM customers
        """)
    params = {}
    with engine.connect() as conn:
        customers = cleanFetchedSQL(
            conn.execute(command, **params).fetchall())
        conn.close()
        return customers


def insertCustomer(email, customer_id, subscription_id, location, trial_end, paid):
    """Adds a customer to the customer sql table, or updates the row if the customer already exists

    Args:
        email (str): Email of the customer
        customer_id (str): Uid of the customer
        subscription_id (str): Uid of the customer's subscription, if applicable
        location (str): The location of the customer (state)
        trial_end (int): The unix timestamp of the expiry of their trial
        paid (bool): Whether or not the user paid before
    """
    command = text("""
        SELECT * FROM customers WHERE "username" = :email
        """)
    params = {'email': email}
    with engine.connect() as conn:
        customers = cleanFetchedSQL(
            conn.execute(command, **params).fetchall())

        if not customers:
            command = text("""
                INSERT INTO customers("username", "id", "subscription", "location", "trial_end", "paid")
                VALUES(:email, :id, :subscription, :location, :trial_end, :paid)
                """)

            params = {'email': email,
                      'id': customer_id,
                      'subscription': subscription_id,
                      'location': location,
                      'trial_end': trial_end,
                      'paid': paid}

            conn.execute(command, **params)
            conn.close()
        else:
            location = customers[0]['location']
            command = text("""
                UPDATE customers
                SET "id" = :id,
                    "subscription" = :subscription,
                    "location" = :location,
                    "trial_end" = :trial_end,
                    "paid" = :paid
                WHERE "username" = :email
                """)

            params = {'email': email,
                      'id': customer_id,
                      'subscription': subscription_id,
                      'location': location,
                      'trial_end': trial_end,
                      'paid': paid}

            conn.execute(command, **params)
            conn.close()


def deleteCustomer(username):
    """Deletes a cusotmer from the table

    Args:
        username (str): The username (email) of the customer
    """
    command = text("""
        DELETE FROM customers WHERE "username" = :email
        """)
    params = {'email': username}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def checkComputer(computer_id, username):
    """TODO: Functions for peer to peer

    Args:
        computer_id ([type]): [description]
        username ([type]): [description]

    Returns:
        [type]: [description]
    """
    command = text("""
        SELECT * FROM studios WHERE "id" = :id AND "username" = :username
        """)
    params = {'id': computer_id, 'username': username}
    with engine.connect() as conn:
        computer = cleanFetchedSQL(
            conn.execute(command, **params).fetchone())
        if not computer:
            computers = fetchComputers(username)
            nicknames = [c['nickname'] for c in computers]
            num = 1
            proposed_nickname = "Computer No. " + str(num)
            while proposed_nickname in nicknames:
                num += 1
                proposed_nickname = "Computer No. " + str(num)

            return {'userName': None,
                    'location': None,
                    'nickname': proposed_nickname,
                    'id': None,
                    'found': False}

        out = {'username': computer[0],
               'location': computer[1],
               'nickname': computer[2],
               'id': computer[3],
               'found': True}
        conn.close()
        return out


def insertComputer(username, location, nickname, computer_id):
    """TODO: Functions for peer to peer

    Args:
        username ([type]): [description]
        location ([type]): [description]
        nickname ([type]): [description]
        computer_id ([type]): [description]
    """
    computer = checkComputer(computer_id, username)
    if not computer['found']:
        command = text("""
            INSERT INTO studios("username", "location", "nickname", "id")
            VALUES(:username, :location, :nickname, :id)
            """)

        params = {'username': username,
                  'location': location,
                  'nickname': nickname,
                  'id': computer_id}

        with engine.connect() as conn:
            conn.execute(command, **params)
            conn.close()


def fetchComputers(username):
    """TODO: Function for peer to peer

    Args:
        username ([type]): [description]

    Returns:
        [type]: [description]
    """
    command = text("""
        SELECT * FROM studios WHERE "username" = :username
        """)
    params = {'username': username}
    with engine.connect() as conn:
        computers = cleanFetchedSQL(
            conn.execute(command, **params).fetchall())
        out = [{'username': computer[0],
                'location': computer[1],
                'nickname': computer[2],
                'id': computer[3]} for computer in computers]
        conn.close()
        return out
    return None


def changeComputerName(username, computer_id, nickname):
    """TODO: Function for peer to peer

    Args:
        username ([type]): [description]
        computer_id ([type]): [description]
        nickname ([type]): [description]
    """
    command = text("""
        UPDATE studios
        SET nickname = :nickname
        WHERE
        "username" = :username
        AND
            "id" = :id
        """)
    params = {'nickname': nickname,
              'username': username, 'id': computer_id}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def fetchAllUsers():
    """Fetches all users from the users sql table

    Returns:
        arr[dict]: The array of users
    """
    command = text("""
        SELECT * FROM users
        """)
    params = {}
    with engine.connect() as conn:
        users = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        conn.close()
        return users
    return None


def mapCodeToUser(code):
    """Returns the user with the respective code. 

    Args:
        code (str): The user's referral code

    Returns:
        dict: The user. If there is no match, return None
    """
    command = text("""
        SELECT * FROM users WHERE "code" = :code
        """)
    params = {'code': code}
    with engine.connect() as conn:
        user = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        return user if user else None


def changeUserCredits(username, credits):
    """Changes the outstanding credits for a user

    Args:
        username (str): The username of the user
        credits (int): The credits that the user has outstanding (1 credit = 1 month of use)
    """
    command = text("""
        UPDATE users
        SET "credits_outstanding" = :credits
        WHERE
        "username" = :username
        """)
    params = {'credits': credits, 'username': username}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def getUserCredits(username):
    """Gets the credits associated with the user

    Args:
        username (str): The user's username

    Returns:
        int: The credits the user has
    """
    command = text("""
        SELECT * FROM users
        WHERE "username" = :username
        """)
    params = {'username': username}
    with engine.connect() as conn:
        users = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        if users:
            return users['credits_outstanding']
    return 0


def fetchCodes():
    """Gets all the referral codes

    Returns:
        arr[str]: An array of all the user codes
    """
    command = text("""
        SELECT * FROM users
        """)
    params = {}
    with engine.connect() as conn:
        users = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        conn.close()
        return [user['code'] for user in users]
    return None


def userVMStatus(username):
    """Returns the status of the user vm

    Args:
        username (string): The username of the user of interest

    Returns:
        str: vm status ['not_created', 'is_creating', 'has_created', 'has_not_paid'] 
    """
    has_paid = False
    has_disk = False

    command = text("""
        SELECT * FROM customers
        WHERE "username" = :username
        """)
    params = {'username': username}
    with engine.connect() as conn:
        user = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        if user:
            has_paid = True

    command = text("""
        SELECT * FROM disks
        WHERE "username" = :username
        """)

    params = {'username': username}
    with engine.connect() as conn:
        user = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        if user:
            has_disk = True

    if not has_paid and not has_disk:
        return 'not_created'

    if has_paid and not has_disk:
        return 'is_creating'

    if has_paid and has_disk:
        return 'has_created'

    return 'has_not_paid'


def checkUserVerified(username):
    """Checks if a user has verified their email already

    Args:
        username (str): The username

    Returns:
        bool: Whether they have verified
    """
    command = text("""
        SELECT * FROM users WHERE "username" = :userName
        """)
    params = {'userName': username}
    with engine.connect() as conn:
        user = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        if user:
            return user['verified']
        return False


def fetchUserToken(username):
    """Returns the uid of the user

    Args:
        username (str): The username of the user

    Returns:
        str: The uid of the user
    """
    command = text("""
        SELECT * FROM users WHERE "username" = :userName
        """)
    params = {'userName': username}
    with engine.connect() as conn:
        user = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        if user:
            return user['id']
        return None


def makeUserVerified(username, verified):
    """Sets the user's verification 

    Args:
        username (str): The username of the user
        verified (bool): The new verification state
    """
    command = text("""
        UPDATE users
        SET verified = :verified
        WHERE
        "username" = :username
        """)
    params = {'verified': verified, 'username': username}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def storeFeedback(username, feedback):
    """Saves feedback in the feedback table

    Args:
        username (str): The username of the user who submitted feedback
        feedback (str): The feedback message
    """

    if feedback:
        command = text("""
            INSERT INTO feedback("username", "feedback")
            VALUES(:email, :feedback)
            """)
        params = {'email': username, 'feedback': feedback}
        with engine.connect() as conn:
            conn.execute(command, **params)
            conn.close()


def updateVMIP(vm_name, ip):
    """Updates the ip address of a vm

    Args:
        vm_name (str): The name of the vm to update
        ip (str): The new ipv4 address
    """
    command = text("""
        UPDATE v_ms
        SET ip = :ip
        WHERE
        "vm_name" = :vm_name
        """)
    params = {'ip': ip, 'vm_name': vm_name}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def updateTrialEnd(subscription, trial_end):
    """Update the end date for the trial subscription

    Args:
        subscription (str): The uid of the subscription we wish to update
        trial_end (int): The trial end date, as a unix timestamp
    """
    command = text("""
        UPDATE customers
        SET trial_end = :trial_end
        WHERE
        "subscription" = :subscription
        """)
    params = {'subscription': subscription, 'trial_end': trial_end}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def updateVMState(vm_name, state):
    """Updates the state for a vm

    Args:
        vm_name (str): The name of the vm to update
        state (str): The new state of the vm
    """
    command = text("""
        UPDATE v_ms
        SET state = :state
        WHERE
        "vm_name" = :vm_name
        """)
    params = {'vm_name': vm_name, 'state': state}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def updateVMStateAutomatically(vm_name, ID=-1):
    """Updates the v_ms sql entry based on the azure vm state

    Args:
        vm_name (str): Name of the vm
        ID (int, optional): Papertrail Logging ID. Defaults to -1.

    Returns:
        int: 1 for success, -1 for error
    """
    _, compute_client, _ = createClients()

    vm_state = compute_client.virtual_machines.instance_view(
        resource_group_name=os.getenv('VM_GROUP'), vm_name=vm_name)

    hr = -1

    try:
        power_state = vm_state.statuses[1].code
        sendInfo(ID, 'VM {} has Azure state {}'.format(
            vm_name, power_state))
    except:
        sendError(
            ID, 'VM {} Azure state unable to be fetched'.format(vm_name))
    if 'starting' in power_state:
        updateVMState(vm_name, 'STARTING')
        sendInfo(ID, 'VM {} set to state {} in Postgres'.format(
            vm_name, 'STARTING'))
        hr = 1
    elif 'running' in power_state:
        updateVMState(vm_name, 'RUNNING_AVAILABLE')
        sendInfo(ID, 'VM {} set to state {} in Postgres'.format(
            vm_name, 'RUNNING_AVAILABLE'))
        hr = 1
    elif 'stopping' in power_state:
        updateVMState(vm_name, 'STOPPING')
        sendInfo(ID, 'VM {} set to state {} in Postgres'.format(
            vm_name, 'STOPPING'))
        hr = 1
    elif 'deallocating' in power_state:
        updateVMState(vm_name, 'DEALLOCATING')
        sendInfo(ID, 'VM {} set to state {} in Postgres'.format(
            vm_name, 'DEALLOCATING'))
        hr = 1
    elif 'stopped' in power_state:
        updateVMState(vm_name, 'STOPPED')
        sendInfo(ID, 'VM {} set to state {} in Postgres'.format(
            vm_name, 'STOPPED'))
        hr = 1
    elif 'deallocated' in power_state:
        updateVMState(vm_name, 'DEALLOCATED')
        sendInfo(ID, 'VM {} set to state {} in Postgres'.format(
            vm_name, 'DEALLOCATED'))
        hr = 1

    return hr


def updateVMLocation(vm_name, location):
    """Updates the location of the vm entry in the v_ms sql table

    Args:
        vm_name (str): Name of vm of interest
        location (str): The new region of the vm
    """
    command = text("""
        UPDATE v_ms
        SET location = :location
        WHERE
        "vm_name" = :vm_name
        """)
    params = {'vm_name': vm_name, 'location': location}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def updateDisk(disk_name, vm_name, location):
    """Updates the vm name and location properties of the disk. If no disk with the provided name exists, create a new disk entry

    Args:
        disk_name (str): Name of disk to update
        vm_name (str): Name of the new vm that the disk is attached to
        location (str): The new Azure region of the disk
    """
    command = text("""
        SELECT * FROM disks WHERE "disk_name" = :disk_name
        """)
    params = {'disk_name': disk_name}
    with engine.connect() as conn:
        disk = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        if disk:
            if location:
                command = text("""
                    UPDATE disks
                    SET "vm_name" = :vm_name, "location" = :location
                    WHERE
                    "disk_name" = :disk_name
                """)
                params = {'vm_name': vm_name,
                          'location': location,
                          'disk_name': disk_name}
            else:
                command = text("""
                    UPDATE disks
                    SET "vm_name" = :vm_name
                    WHERE
                    "disk_name" = :disk_name
                """)
                params = {'vm_name': vm_name,
                          'disk_name': disk_name}
        else:
            if location:
                command = text("""
                    INSERT INTO disks("disk_name", "vm_name", "location") 
                    VALUES(:disk_name, :vm_name, :location)
                    """)
                params = {'vm_name': vm_name,
                          'location': location,
                          'disk_name': disk_name}
            else:
                command = text("""
                    INSERT INTO disks("disk_name", "vm_name") 
                    VALUES(:disk_name, :vm_name)
                    """)
                params = {'vm_name': vm_name,
                          'disk_name': disk_name}

        conn.execute(command, **params)
        conn.close()


def assignUserToDisk(disk_name, username):
    """Assigns a user to a disk

    Args:
        disk_name (str): Disk that the user will be assigned to
        username (str): Username of the user
    """
    command = text("""
        UPDATE disks SET "username" = :username WHERE "disk_name" = :disk_name
        """)
    params = {'username': username, 'disk_name': disk_name}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def fetchAttachableVMs(state, location):
    """Finds all vms with specified location and state that are unlocked and not in dev mode

    Args:
        state (str): State of the vm
        location (str): Azure region to look in

    Returns:
        arr[dict]: Arrray of all vms that are attachable
    """
    command = text("""
        SELECT * FROM v_ms WHERE "state" = :state AND "location" = :location AND "lock" = :lock AND "dev" = :dev
        """)
    params = {'state': state, 'location': location,
              'lock': False, 'dev': False}

    with engine.connect() as conn:
        vms = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        conn.close()
        vms = vms if vms else []
        return vms


def getMostRecentActivity(username):
    """Gets the last activity of a user

    Args:
        username (str): Username of the user

    Returns:
        str: The latest activity of the user
    """
    command = text("""
        SELECT *
        FROM login_history
        WHERE "username" = :username
        ORDER BY timestamp DESC LIMIT 1
        """)

    params = {'username': username}

    with engine.connect() as conn:
        activity = cleanFetchedSQL(
            conn.execute(command, **params).fetchone())
        return activity


def addPendingCharge(username, amount, ID=0):
    """Adds to the user's current pending charges by amount. This is done since stripe requires payments of at least 50 cents. By using pending charges, we can keep track and charge it all at the end.

    Args:
        username (str): The username of the user to add a pending charge to
        amount (int): Amount by which to increment by
        ID (int, optional): Papertrail logging ID. Defaults to 0.
    """
    command = text("""
        SELECT *
        FROM customers
        WHERE "username" = :username
        """)

    params = {'username': username}

    with engine.connect() as conn:
        customer = cleanFetchedSQL(
            conn.execute(command, **params).fetchone())
        if customer:
            pending_charges = customer['pending_charges'] + amount
            command = text("""
                UPDATE customers
                SET "pending_charges" = :pending_charges
                WHERE "username" = :username
                """)
            params = {'pending_charges': pending_charges,
                      'username': username}

            conn.execute(command, **params)
            conn.close()
        else:
            sendCritical(
                ID, '{} has an hourly plan but was not found as a customer in database'.format(username))


def associateVMWithDisk(vm_name, disk_name):
    """Associates a VM with a disk on the v_ms sql table

    Args:
        vm_name (str): The name of the vm
        disk_name (str): The name of the disk
    """
    username = mapDiskToUser(disk_name)
    username = username if username else ''
    command = text("""
        UPDATE v_ms
        SET "disk_name" = :disk_name, "username" = :username
        WHERE
        "vm_name" = :vm_name
        """)
    params = {'vm_name': vm_name,
              'disk_name': disk_name, 'username': username}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def lockVM(vm_name, lock, username = None, disk_name = None, change_last_updated = True, verbose = True, ID=-1):
    """Locks/unlocks a vm. A vm entry with lock set to True prevents other processes from changing that entry.

    Args:
        vm_name (str): The name of the vm to lock
        lock (bool): True for lock
        change_last_updated (bool, optional): Whether or not to change the last_updated column as well. Defaults to True.
        verbose (bool, optional): Flag to log verbose in papertrail. Defaults to True.
        ID (int, optional): A unique papertrail logging id. Defaults to -1.
    """
    if lock and verbose:
        sendInfo(ID, 'Trying to lock VM {}'.format(
            vm_name), papertrail=verbose)
    elif not lock and verbose:
        sendInfo(ID, 'Trying to unlock VM {}'.format(
            vm_name), papertrail=verbose)

    session = Session()

    command = text("""
        UPDATE v_ms
        SET "lock" = :lock, "last_updated" = :last_updated
        WHERE
        "vm_name" = :vm_name
        """)

    if not change_last_updated:
        command = text("""
            UPDATE v_ms
            SET "lock" = :lock
            WHERE
            "vm_name" = :vm_name
            """)

    last_updated = getCurrentTime()
    params = {'vm_name': vm_name, 'lock': lock,
              'last_updated': last_updated}

    session.execute(command, params)

    if username and disk_name:
        command = text("""
            UPDATE v_ms
            SET "username" = :username, "disk_name" = :disk_name
            WHERE
            "vm_name" = :vm_name
            """)

        params = {'username': username, 'vm_name': vm_name, 'disk_name': disk_name}
        session.execute(command, params)

    session.commit()
    session.close()

    if lock and verbose:
        sendInfo(ID, 'Successfully locked VM {}'.format(
            vm_name), papertrail=verbose)
    elif not lock and verbose:
        sendInfo(ID, 'Successfully unlocked VM {}'.format(
            vm_name), papertrail=verbose)


def claimAvailableVM(disk_name, location, ID = -1):
    username = mapDiskToUser(disk_name)
    session = Session()

    state_preference = ['RUNNING_AVAILABLE', 'STOPPED', 'DEALLOCATED']

    for state in state_preference:
        sendInfo(ID, 'Looking for VMs with state {} in {}'.format(state, location))

        command = text("""
            SELECT * FROM v_ms
            WHERE lock = :lock AND state = :state AND dev = :dev AND location = :location AND (temporary_lock < :temporary_lock OR temporary_lock IS NULL)
            """)

        params = {'lock': False, 'state': state, 'dev': False, 'location': location, 'temporary_lock': dateToUnix(getToday())}

        available_vm = cleanFetchedSQL(session.execute(command, params).fetchone())

        if available_vm:
            sendInfo(ID, 'Found an available VM {}'.format(str(available_vm)))

            command = text("""
                UPDATE v_ms 
                SET lock = :lock, username = :username, disk_name = :disk_name
                WHERE vm_name = :vm_name
                """)

            params = {'lock': True, 'username': username, 'disk_name': disk_name, 'vm_name': available_vm['vm_name']}
            session.execute(command, params)
            session.commit()
            session.close()

            return available_vm
        else:
            sendInfo(ID, 'Did not find any VMs in {} with state {}.'.format(location, state))

    session.commit()
    session.close()
    return None

def createTemporaryLock(vm_name, minutes, ID = -1):
    temporary_lock = shiftUnixByMinutes(dateToUnix(getToday()), minutes)
    session = Session()

    command = text("""
        UPDATE v_ms
        SET "temporary_lock" = :temporary_lock
        WHERE
        "vm_name" = :vm_name
        """)

    params = {'vm_name': vm_name, 'temporary_lock': temporary_lock}
    session.execute(command, params)

    sendInfo(ID, 'Temporary lock created for VM {} for {} minutes'.format(vm_name, str(minutes)))

    session.commit()
    session.close()


def vmReadyToConnect(vm_name, ready):
    """Sets the vm's ready_to_connect field

    Args:
        vm_name (str): Name of the vm
        ready (boolean): True for ready to connect
    """
    command = text("""
        UPDATE v_ms
        SET "ready_to_connect" = :ready
        WHERE
        "vm_name" = :vm_name
        """)
    params = {'vm_name': vm_name, 'ready': ready}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def checkLock(vm_name, ID = -1):
    """Check to see if a vm has been locked

    Args:
        vm_name (str): Name of the vm to check

    Returns:
        bool: True if VM is locked, False otherwise
    """
    session = Session()

    command = text("""
        SELECT * FROM v_ms WHERE "vm_name" = :vm_name
        """)
    params = {'vm_name': vm_name}

    vm = cleanFetchedSQL(session.execute(command, params).fetchone())
    session.commit()
    session.close()

    if vm:
        temporary_lock = False
        if vm['temporary_lock']:
            temporary_lock = dateToUnix(getToday()) < vm['temporary_lock']
            sendInfo(ID, 'Temporary lock found on VM {}, expires at {}'.format(vm_name, str(unixToDate(vm['temporary_lock']))))
        return vm['lock'] or temporary_lock
    return None


def checkDev(vm_name):
    """Checks to see if a vm is in dev mode

    Args:
        vm_name (str): Name of vm to check

    Returns:
        bool: True if vm is in dev mode, False otherwise
    """
    command = text("""
        SELECT * FROM v_ms WHERE "vm_name" = :vm_name
        """)
    params = {'vm_name': vm_name}

    with engine.connect() as conn:
        vm = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        if vm:
            return vm['dev']
        return None


def checkWinlogon(vm_name):
    """Checks if a vm is ready to connect

    Args:
        vm_name (str): Name of the vm to check

    Returns:
        bool: True if vm is ready to connect
    """
    command = text("""
        SELECT * FROM v_ms WHERE "vm_name" = :vm_name
        """)
    params = {'vm_name': vm_name}

    with engine.connect() as conn:
        vm = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        if vm:
            return vm['ready_to_connect']
        return None


def attachDiskToVM(disk_name, vm_name, lun, ID=-1):
    """Attach a secondary disk to a vm

    Args:
        disk_name (str): Name of the disk
        vm_name (str): Name of the vm
        lun (int): The logical unit number of the disk.  This value is used to identify data disks within the VM and therefore must be unique for each data disk attached to a VM.
        ID (int, optional): The unique papertrail logging id. Defaults to -1.

    Returns:
        int: ! for success, -1 for fail
    """
    try:
        _, compute_client, _ = createClients()
        virtual_machine = getVM(vm_name)

        data_disk = compute_client.disks.get(
            os.getenv('VM_GROUP'), disk_name)
        virtual_machine.storage_profile.data_disks.append({
            'lun': lun,
            'name': disk_name,
            'create_option': DiskCreateOption.attach,
            'managed_disk': {
                'id': data_disk.id
            }
        })

        async_disk_attach = compute_client.virtual_machines.create_or_update(
            os.getenv('VM_GROUP'),
            virtual_machine.name,
            virtual_machine
        )
        async_disk_attach.wait()
        return 1
    except Exception as e:
        sendCritical(ID, str(e))
        return -1


def swapdisk_name(s, disk_name, vm_name, ID=-1):
    """Attaches an OS disk to the VM. If the vm already has an OS disk attached, the vm will detach that and attach this one.

    Args:
        s (class): The reference to the parent's instance of self
        disk_name (str): The name of the disk
        vm_name (str): The name of the vm
        ID (int, optional): The unique papertrail logging ID. Defaults to -1.

    Returns:
        [type]: [description]
    """
    try:
        _, compute_client, _ = createClients()
        virtual_machine = getVM(vm_name)
        new_os_disk = compute_client.disks.get(
            os.getenv('VM_GROUP'), disk_name)

        virtual_machine.storage_profile.os_disk.managed_disk.id = new_os_disk.id
        virtual_machine.storage_profile.os_disk.name = new_os_disk.name

        sendInfo(ID, 'Attaching disk {} to {}'.format(disk_name, vm_name))
        start = time.perf_counter()
        async_disk_attach = compute_client.virtual_machines.create_or_update(
            os.getenv('VM_GROUP'), vm_name, virtual_machine
        )
        sendInfo(ID, async_disk_attach.result())
        end = time.perf_counter()
        sendInfo(ID, 'Disk {} attached to VM {} in {} seconds'.format(
            disk_name, vm_name, str(end-start)))

        s.update_state(state='PENDING', meta={
            "msg": "Data successfully uploaded to server. Starting server."})

        return fractalVMStart(vm_name, True)
    except Exception as e:
        sendCritical(ID, str(e))
        return -1


def fetchAllDisks():
    """Fetches all the disks

    Returns:
        arr[dict]: An array of all the disks in the disks sql table
    """
    command = text("""
        SELECT *
        FROM disks
        """)

    params = {}

    with engine.connect() as conn:
        disks = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        return disks


def deleteDiskFromTable(disk_name):
    """Deletes a disk from the disks sql table

    Args:
        disk_name (str): The name of the disk to delete
    """
    command = text("""
        DELETE FROM disks WHERE "disk_name" = :disk_name 
        """)
    params = {'disk_name': disk_name}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def scheduleDiskDelete(disk_name, date, ID=-1):
    """Schedule a disk to be deleted later

    Args:
        disk_name (str): Name of the disk to be deleted
        date (str): The date at which it should be deleted. In the format mm/dd/yyyy, hh:mm (24 hour time)
        ID (int, optional): The unique paprtrail loggin id. Defaults to -1.
    """
    dateString = dateToString(date)
    command = text("""
        UPDATE disks
        SET "delete_date" = :delete_date, "state" = :'TO_BE_DELETED'
        WHERE
        "disk_name" = :disk_name
        """)
    params = {'disk_name': disk_name, 'delete_date': dateString}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()
    sendInfo(ID, 'Fetching all disks associated with state ACTIVE')


def mapDiskToVM(disk_name):
    """Find the vm with the specified disk attached

    Args:
        disk_name (str): Name of the disk to look for

    Returns:
        dict: The vm with the specified disk attached
    """
    command = text("""
        SELECT * FROM v_ms WHERE "disk_name" = :disk_name
        """)

    params = {'disk_name': disk_name}

    with engine.connect() as conn:
        vms = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        return vms


def updateVM(vm_name, location, disk_name, username):
    """Updates the vm entry in the vm_s sql table

    Args:
        vm_name (str): The name of the vm to update
        location (str): The new Azure region of the vm
        disk_name (str): The new disk that is attached to the vm
        username (str): The new username associated with the vm
    """
    command = text("""
        UPDATE v_ms
        SET "location" = :location, "disk_name" = :disk_name, "username" = :username
        WHERE
        "vm_name" = :vm_name
        """)
    params = {'location': location, 'vm_name': vm_name,
              'disk_name': disk_name, 'username': username}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def mapDiskToUser(disk_name, ID=-1):
    """Find the user that is associated with the disk

    Args:
        disk_name (str): The name of the disk of interest
        ID (int, optional): Papertrail loggin id. Defaults to -1.

    Returns:
        str: The username associated with the disk
    """
    command = text("""
        SELECT * FROM disks WHERE "disk_name" = :disk_name
        """)

    params = {'disk_name': disk_name}

    with engine.connect() as conn:
        disk = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        if disk:
            sendInfo(ID, 'Disk {} belongs to user {}'.format(
                disk_name, disk['username']))
            return disk['username']
        sendWarning(ID, 'No username found for disk {}'.format(disk_name))
        return None


def deleteVMFromTable(vm_name):
    """Deletes a vm from the v_ms sql table

    Args:
        vm_name (str): The name of the vm to delete
    """
    command = text("""
        DELETE FROM v_ms WHERE "vm_name" = :vm_name 
        """)
    params = {'vm_name': vm_name}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def updateDiskState(disk_name, state):
    """Updates the state of a disk in the disks sql table

    Args:
        disk_name (str): Name of the disk to update
        state (str): The new state of the disk
    """
    command = text("""
        UPDATE disks
        SET state = :state
        WHERE
        "disk_name" = :disk_name
        """)
    params = {'state': state, 'disk_name': disk_name}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def assignVMSizeToDisk(disk_name, vm_size):
    """Updates the vm_size field for a disk

    Args:
        disk_name (str): The name of the disk to update
        vm_size (str): The new size of the vm the disk is attached to
    """
    command = text("""
        UPDATE disks SET "vm_size" = :vm_size WHERE "disk_name" = :disk_name
        """)
    params = {'vm_size': vm_size, 'disk_name': disk_name}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def createDiskFromImageHelper(username, location, vm_size, ID=-1):
    """Creates a disk from an image, and assigns users and vm_sizes to the disk

    Args:
        username (str): The username of the user that will be assigned to the disk
        location (str): The Azure region of the new disk
        vm_size (str): The size of the vm associated with the new disk
        ID (int, optional): Unique papertrail logging id. Defaults to -1.

    Returns:
        int: 200 for success, 400 for error
    """
    disk_name = genDiskName()
    sendInfo(ID, 'Preparing to create disk {} from an image'.format(disk_name))
    _, compute_client, _ = createClients()

    try:
        ORIGINAL_DISK = 'Fractal_Disk_Eastus'
        if location == 'southcentralus':
            ORIGINAL_DISK = 'Fractal_Disk_Southcentralus'
        elif location == 'northcentralus':
            ORIGINAL_DISK = 'Fractal_Disk_Northcentralus'

        disk_image = compute_client.disks.get('Fractal', ORIGINAL_DISK)
        sendInfo(ID, 'Image found. Preparing to create disk {} with location {} under {} attached to a {} VM'.format(
            disk_name, location, username, vm_size))
        async_disk_creation = compute_client.disks.create_or_update(
            'Fractal',
            disk_name,
            {
                'location': location,
                'creation_data': {
                    'create_option': DiskCreateOption.copy,
                    'source_resource_id': disk_image.id
                }
            }
        )
        sendInfo(ID, 'Disk clone command sent. Waiting on disk to create')
        async_disk_creation.wait()
        sendInfo(
            ID, 'Disk {} successfully created from image'.format(disk_name))
        new_disk = async_disk_creation.result()

        updateDisk(disk_name, '', location)
        assignUserToDisk(disk_name, username)
        assignVMSizeToDisk(disk_name, vm_size)

        return {'status': 200, 'disk_name': disk_name}
    except Exception as e:
        sendCritical(ID, str(e))

        sendInfo(ID, 'Attempting to delete the disk {}'.format(disk_name))
        os_disk_delete = compute_client.disks.delete(
            os.getenv('VM_GROUP'), disk_name)
        os_disk_delete.wait()
        sendInfo(
            ID, 'Disk {} deleted due to a critical error in disk creation'.format(disk_name))

        time.sleep(30)
        return {'status': 400, 'disk_name': None}


def sendVMStartCommand(vm_name, needs_restart, ID=-1):
    """Starts a vm

    Args:
        vm_name (str): The name of the vm to start
        needs_restart (bool): Whether the vm needs to restart after
        ID (int, optional): Unique papertrail logging id. Defaults to -1.

    Returns:
        int: 1 for success, -1 for fail
    """
    try:
        def boot_if_necessary(vm_name, needs_restart, ID):
            _, compute_client, _ = createClients()

            power_state = 'PowerState/deallocated'
            vm_state = compute_client.virtual_machines.instance_view(
                resource_group_name=os.getenv('VM_GROUP'), vm_name=vm_name)

            try:
                power_state = vm_state.statuses[1].code
            except Exception as e:
                sendCritical(ID, str(e))
                pass

            if 'stop' in power_state or 'dealloc' in power_state:
                sendInfo(ID, 'VM {} currently in state {}. Setting Winlogon to False'.format(
                    vm_name, power_state))
                vmReadyToConnect(vm_name, False)
                sendInfo(ID, 'Starting VM {}'.format(vm_name))
                updateVMState(vm_name, 'STARTING')
                lockVM(vm_name, True, ID = ID)

                async_vm_start = compute_client.virtual_machines.start(
                    os.environ.get('VM_GROUP'), vm_name)

                sendInfo(ID, async_vm_start.result())
                sendInfo(ID, 'VM {} started successfully'.format(vm_name))

            if needs_restart:
                sendInfo(
                    ID, 'VM {} needs to restart. Setting Winlogon to False'.format(vm_name))
                vmReadyToConnect(vm_name, False)

                updateVMState(vm_name, 'RESTARTING')
                lockVM(vm_name, True, ID = ID)

                async_vm_restart = compute_client.virtual_machines.restart(
                    os.environ.get('VM_GROUP'), vm_name)

                sendInfo(ID, async_vm_restart.result())
                sendInfo(ID, 'VM {} restarted successfully'.format(vm_name))

        boot_if_necessary(vm_name, needs_restart, ID)
        updateVMState(vm_name, 'RUNNING_AVAILABLE')
        lockVM(vm_name, False, ID = ID)

        winlogon = waitForWinlogon(vm_name, ID)
        while winlogon < 0:
            boot_if_necessary(vm_name, True, ID)
            winlogon = waitForWinlogon(vm_name, ID)

        return 1
    except Exception as e:
        sendCritical(ID, str(e))
        return -1


def waitForWinlogon(vm_name, ID = -1):
    """Periodically checks and sleeps until winlogon succeeds

    Args:
        vm_name (str): Name of the vm
        ID (int, optional): Unique papertrail logging id. Defaults to -1.

    Returns:
        int: 1 for success, -1 for fail
    """
    ready = checkWinlogon(vm_name)

    num_tries = 0

    if ready:
        sendInfo(ID, 'VM {} has Winlogoned successfully'.format(vm_name))
        return 1

    if checkDev(vm_name):
        sendInfo(
            ID, 'VM {} is a DEV machine. Bypassing Winlogon. Sleeping for 50 seconds before returning.'.format(vm_name))
        time.sleep(50)
        return 1

    while not ready:
        sendWarning(ID, 'Waiting for VM {} to Winlogon'.format(vm_name))
        time.sleep(5)
        ready = checkWinlogon(vm_name)
        num_tries += 1

        if num_tries > 20:
            sendError(ID, 'Waited too long for winlogon. Sending failure message.')
            return -1

    sendInfo(ID, 'VM {} has Winlogon successfully after {} tries'.format(vm_name, str(num_tries)))

    return 1


def fractalVMStart(vm_name, needs_restart=False, ID=-1):
    """Bullies Azure into actually starting the vm by repeatedly calling sendVMStartCommand if necessary (big brain thoughts from Ming)

    Args:
        vm_name (str): Name of the vm to start
        needs_restart (bool, optional): Whether the vm needs to restart after. Defaults to False.
        ID (int, optional): Unique papertrail logging id. Defaults to -1.

    Returns:
        int: 1 for success, -1 for failure
    """
    _, compute_client, _ = createClients()

    started = False
    start_attempts = 0

    # We will try to start/restart the VM and wait for it three times in total before giving up
    while not started and start_attempts < 3:
        start_command_tries = 0

        # First, send a basic start or restart command. Try six times, if it fails, give up
        while sendVMStartCommand(vm_name, needs_restart) < 0 and start_command_tries < 6:
            time.sleep(10)
            start_command_tries += 1

        if start_command_tries >= 6:
            return -1

        wake_retries = 0

        # After the VM has been started/restarted, query the state. Try 12 times for the state to be running. If it is not running,
        # give up and go to the top of the while loop to send another start/restart command
        vm_state = compute_client.virtual_machines.instance_view(
            resource_group_name=os.getenv('VM_GROUP'), vm_name=vm_name)

        # Success! VM is running and ready to use
        if 'running' in vm_state.statuses[1].code:
            updateVMState(vm_name, 'RUNNING_AVAILABLE')
            sendInfo(ID, 'Running found in status of VM {}'.format(vm_name))
            started = True
            return 1

        while not 'running' in vm_state.statuses[1].code and wake_retries < 12:
            sendWarning(ID, 'VM state is currently in state {}, sleeping for 5 seconds and querying state again'.format(
                vm_state.statuses[1].code))
            time.sleep(5)
            vm_state = compute_client.virtual_machines.instance_view(
                resource_group_name=os.getenv('VM_GROUP'), vm_name=vm_name)

            # Success! VM is running and ready to use
            if 'running' in vm_state.statuses[1].code:
                updateVMState(vm_name, 'RUNNING_AVAILABLE')
                sendInfo(ID, 'VM {} is running. State is {}'.format(
                    vm_name, vm_state.statuses[1].code))
                started = True
                return 1

            wake_retries += 1

        start_attempts += 1

    return -1


def spinLock(vm_name, ID = -1):
    """Waits for vm to be unlocked

    Args:
        vm_name (str): Name of vm of interest
        ID (int, optional): Unique papertrail logging id. Defaults to -1.

    Returns:
        int: 1 = vm is unlocked, -1 = giving up
    """
    locked = checkLock(vm_name)

    num_tries = 0

    if not locked:
        sendInfo(ID, 'VM {} is unlocked.'.format(vm_name))
        return 1

    while locked:
        sendWarning(
            ID, 'VM {} is locked. Waiting to be unlocked.'.format(vm_name))
        time.sleep(5)
        locked = checkLock(vm_name)
        num_tries += 1

        if num_tries > 20:
            sendCritical(
                ID, 'FAILURE: VM {} is locked for too long. Giving up.'.format(vm_name))
            return -1

    sendInfo(ID, 'After waiting {} times, VM {} is unlocked'.format(
        num_tries, vm_name))
    return 1


def updateProtocolVersion(vm_name, version):
    """Updates the protocol version associated with the vm. This is used for tracking updates.

    Args:
        vm_name (str): The name of the vm to update
        version (str): The id of the new version. It is based off a github commit number.
    """
    vm = getVM(vm_name)
    os_disk = vm.storage_profile.os_disk.name

    command = text("""
        UPDATE disks
        SET "version" = :version
        WHERE
           "disk_name" = :disk_name
    """)
    params = {'version': version,
              'disk_name': os_disk}

    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()
