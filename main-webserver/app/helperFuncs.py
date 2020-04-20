from .utils import *
from app import engine


def createClients():
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


def deleteResource(name, delete_disk):
    _, compute_client, network_client = createClients()
    vnetName, subnetName, ipName, nicName = name + \
        '_vnet', name + '_subnet', name + '_ip', name + '_nic'
    hr = 1

    # get VM info based on name 
    virtual_machine = getVM(name)
    os_disk_name = virtual_machine.storage_profile.os_disk.name

    # step 1, deallocate the VM
    try:
        print("Attempting to deallocate the VM...")
        async_vm_deallocate = compute_client.virtual_machines.deallocate(
            os.getenv('VM_GROUP'), name)
        async_vm_deallocate.wait()
        time.sleep(60) # wait a whole minute to ensure it deallocated properly
        print("VM deallocated")
    except Exception as e:
        print(e)
        hr = -1
  
    # step 2, detach the IP
    try:
        print("Attempting to detach the IP...")
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
        print("IP detached")
    except Exception as e:
        print(e)
        hr = -1

    # step 3, delete the VM
    try:
        print("Attempting to delete the VM...")
        async_vm_delete = compute_client.virtual_machines.delete(
            os.getenv('VM_GROUP'), name)
        async_vm_delete.wait()
        print("VM deleted")
        deleteVMFromTable(name)
        print("VM removed from database")
    except Exception as e:
        print(e)
        hr = -1

    # step 4, delete the IP
    try:
        print("Attempting to delete the IP...")
        async_ip_delete = network_client.public_ip_addresses.delete(
            os.getenv('VM_GROUP'),
            ipName
        )
        async_ip_delete.wait()
        print("IP deleted")
    except Exception as e:
        print(e)
        hr = -1

    # step 4, delete the NIC
    try:
        print("Attempting to delete the NIC...")
        async_nic_delete = network_client.network_interfaces.delete(
            os.getenv('VM_GROUP'),
            nicName
        )
        async_nic_delete.wait()
        print("NIC deleted")
    except Exception as e:
        print(e)
        hr = -1

    # step 5, delete the Vnet
    try:
        print("Attempting to delete the Vnet...")
        async_vnet_delete = network_client.virtual_networks.delete(
            os.getenv('VM_GROUP'),
            vnetName
        )
        async_vnet_delete.wait()
        print("Vnet deleted")
    except Exception as e:
        print(e)
        hr = -1

    if delete_disk:
        # step 6, delete the OS disk -- not needed anymore (OS disk swapping)
        try:
            print("Attempting to delete the OS disk...")
            os_disk_delete = compute_client.disks.delete(
                os.getenv('VM_GROUP'), os_disk_name)
            os_disk_delete.wait()
            print("OS disk deleted")
        except Exception as e:
            print(e)
            hr = -1

    return hr


def createVMParameters(vmName, nic_id, vm_size, location):
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
        }

        command = text("""
            INSERT INTO v_ms("vm_name", "username", "disk_name") 
            VALUES(:vmName, :username, :disk_name)
            """)
        params = {'vmName': vmName, 'username': userName, 'disk_name': None}
        with engine.connect() as conn:
            conn.execute(command, **params)
            conn.close()
            return {'params': {
                'location': location,
                'os_profile': {
                    'computer_name': vmName,
                    'admin_username': os.getenv('VM_GROUP'),
                    'admin_password': os.getenv('VM_PASSWORD')
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
                        'os_type': 'Windows',
                        'create_option': 'FromImage',
                        'caching': 'ReadOnly'
                    }
                },
                'network_profile': {
                    'network_interfaces': [{
                        'id': nic_id,
                    }]
                },
            }, 'vm_name': vmName}


def createDiskEntry(disk_name, vm_name, username, location):
    with engine.connect() as conn:
        command = text("""
            INSERT INTO disks("disk_name", "vm_name", "username", "location") 
            VALUES(:diskname, :vmname, :username, :location)
            """)
        params = {
            'diskname': disk_name,
            'vmname': vm_name,
            'username': username,
            "location": location
        }
        with engine.connect() as conn:
            conn.execute(command, **params)
            conn.close()


def getVM(vm_name):
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
    command = text("""
        SELECT * FROM v_ms WHERE "vm_name" = :value
        """)
    params = {'value': value}
    with engine.connect() as conn:
        exists = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        conn.close()
        return True if exists else False


def getIP(vm):
    _, _, network_client = createClients()
    ni_reference = vm.network_profile.network_interfaces[0]
    ni_reference = ni_reference.id.split('/')
    ni_group = ni_reference[4]
    ni_name = ni_reference[8]

    net_interface = network_client.network_interfaces.get(ni_group, ni_name)
    ip_reference = net_interface.ip_configurations[0].public_ip_address
    ip_reference = ip_reference.id.split('/')
    ip_group = ip_reference[4]
    ip_name = ip_reference[8]

    public_ip = network_client.public_ip_addresses.get(ip_group, ip_name)
    return public_ip.ip_address


def updateVMUsername(username, vm_name):
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


def loginUserVM(username):
    command = text("""
        SELECT * FROM v_ms WHERE "username" = :username
        """)
    params = {'username': username}
    with engine.connect() as conn:
        users = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        conn.close()
        if users:
            return user[0]['username']
    return None


def loginUser(username, password):
    if password != os.getenv('ADMIN_PASSWORD'):
        command = text("""
            SELECT * FROM users WHERE "username" = :userName AND "password" = :password
            """)
        pwd_token = jwt.encode({'pwd': password}, os.getenv('SECRET_KEY'))
        params = {'userName': username, 'password': pwd_token}
        with engine.connect() as conn:
            user = cleanFetchedSQL(conn.execute(command, **params).fetchall())
            conn.close()
            return True if user else False
    else:
        command = text("""
            SELECT * FROM users WHERE "username" = :userName
            """)
        params = {'userName': username}
        with engine.connect() as conn:
            user = cleanFetchedSQL(conn.execute(command, **params).fetchall())
            conn.close()
            return True if user else False


def lookup(username):
    command = text("""
        SELECT * FROM users WHERE "username" = :userName
        """)
    params = {'userName': username}
    with engine.connect() as conn:
        user = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        conn.close()
        return True if user else False


def genUniqueCode():
    with engine.connect() as conn:
        old_codes = [cell[0]
                     for cell in list(conn.execute('SELECT "code" FROM users'))]
        new_code = generateCode()
        while new_code in old_codes:
            new_code = generateCode()
        return new_code


def registerUser(username, password, token):
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


def regenerateAllCodes():
    with engine.connect() as conn:
        for row in list(conn.execute('SELECT * FROM users')):
            code = genUniqueCode()
            command = text("""
                UPDATE users 
                SET "code" = :code
                WHERE "username" = :userName
                """)
            params = {'code': code, 'userName': row[0]}
            conn.execute(command, **params)
            conn.close()


def generateIDs():
    with engine.connect() as conn:
        for row in list(conn.execute('SELECT * FROM users')):
            token = generateToken(row[0])
            print("THE USER IS " + row[0])
            print("THE TOKEN IS " + token)
            command = text("""
                UPDATE users 
                SET "id" = :token
                WHERE "username" = :userName
                """)
            params = {'token': token, 'userName': row[0]}
            conn.execute(command, **params)
            conn.close()


def resetPassword(username, password):
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
    command = text("""
        SELECT * FROM v_ms WHERE "vm_name" = :vm_name
        """)
    params = {'vm_name': vm_name}
    with engine.connect() as conn:
        vm_info = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        # Decode password
        conn.close()
        return vm_info


def genVMName():
    with engine.connect() as conn:
        oldVMs = [cell[0]
                  for cell in list(conn.execute('SELECT "vm_name" FROM v_ms'))]
        vmName = genHaiku(1)[0]
        while vmName in oldVMs:
            vmName = genHaiku(1)[0]
        return vmName


def genDiskName():
    with engine.connect() as conn:
        oldDisks = [cell[0] for cell in list(
            conn.execute('SELECT "disk_name" FROM disks'))]
        diskName = genHaiku(1)[0]
        while diskName in oldDisks:
            diskName = genHaiku(1)[0]
        return str(diskName)


def storeForm(name, email, cubeType):
    command = text("""
        INSERT INTO form("fullname", "username", "cubetype") 
        VALUES(:name, :email, :cubeType)
        """)
    params = {'name': name, 'email': email, 'cubeType': cubeType}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def storePreOrder(address1, address2, zipCode, email, order):
    command = text("""
        INSERT INTO pre_order("address1", "address2", "zipcode", "username", "base", "enhanced", "power") 
        VALUES(:address1, :address2, :zipcode, :email, :base, :enhanced, :power)
        """)
    params = {'address1': address1, 'address2': address2, 'zipcode': zipCode, 'email': email,
              'base': int(order['base']), 'enhanced': int(order['enhanced']), 'power': int(order['power'])}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def addTimeTable(username, action, time, is_user):
    command = text("""
        INSERT INTO login_history("username", "timestamp", "action", "is_user") 
        VALUES(:userName, :currentTime, :action, :is_user)
        """)

    tz = pytz.timezone("US/Eastern")
    aware = tz.localize(dt.now(), is_dst=None)
    now = aware.strftime('%m-%d-%Y, %H:%M:%S')
    params = {'userName': username, 'currentTime': now, 
              'action': action, 'is_user': is_user}

    with engine.connect() as conn:
        conn.execute(command, **params)

        disk = fetchUserDisks(username)
        if disk:
            disk_name = disk[0]['disk_name']
            vms = mapDiskToVM(disk_name)
            if vms:
                _, compute_client, _ = createClients()
                vm_name = vms[0]['vm_name']
                vm_state = compute_client.virtual_machines.instance_view(
                    resource_group_name = os.getenv('VM_GROUP'), vm_name = vm_name)
                if 'running' in vm_state.statuses[1].code:
                    state = 'RUNNING_AVAILABLE' if action == 'logoff' else 'RUNNING_UNAVAILABLE'
                    updateVMState(vms[0]['vm_name'], state)
                else:
                    state = 'NOT_RUNNING_AVAILABLE' if action == 'logoff' else 'NOT_RUNNING_UNAVAILABLE'
                    updateVMState(vms[0]['vm_name'], state)  
            else:
                print("CRITICAL ERROR: Could not find a VM currently attached to disk " + disk_name)
        else:
            print("CRITICAL ERROR: Could not find disk in database attached to user " + username)

        conn.close()

def deleteTimeTable():
    command = text("""
        DELETE FROM login_history
        """)
    params = {}

    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def fetchUserVMs(username):
    if(username):
        command = text("""
            SELECT * FROM v_ms WHERE "username" = :username
            """)
        params = {'username': username}
        with engine.connect() as conn:
            vms_info = cleanFetchedSQL(conn.execute(command, **params).fetchall())
            conn.close()
            return vms_info
    else:
        command = text("""
            SELECT * FROM v_ms
            """)
        params = {}
        with engine.connect() as conn:
            vms_info = cleanFetchedSQL(conn.execute(command, **params).fetchall())
            conn.close()
            return vms_info


def fetchUserDisks(username, show_all = False):
    if username:
        if not show_all:
            command = text("""
                SELECT * FROM disks WHERE "username" = :username AND "state" = :state
                """)
            params = {'username': username, 'state': 'ACTIVE'}
            with engine.connect() as conn:
                disks_info = cleanFetchedSQL(conn.execute(command, **params).fetchall())
                conn.close()
                return disks_info
        else:
            command = text("""
                SELECT * FROM disks WHERE "username" = :username
                """)
            params = {'username': username}
            with engine.connect() as conn:
                disks_info = cleanFetchedSQL(conn.execute(command, **params).fetchall())
                conn.close()
                return disks_info
    else:
        command = text("""
            SELECT * FROM disks
            """)
        params = {}
        with engine.connect() as conn:
            disks_info = cleanFetchedSQL(conn.execute(command, **params).fetchall())
            conn.close()
            return disks_info


def fetchUserCode(username):
    try:
        command = text("""
            SELECT * FROM users WHERE "username" = :userName
            """)
        params = {'userName': username}
        with engine.connect() as conn:
            user = cleanFetchedSQL(conn.execute(command, **params).fetchone())
            conn.close()
            return user['code']
    except:
        return None


def deleteRow(username, vm_name, usernames, vm_names):
    if not (vm_name in vm_names):
        command = text("""
            DELETE FROM v_ms WHERE "vm_name" = :vm_name 
            """)
        params = {'vm_name': vm_name}
        with engine.connect() as conn:
            conn.execute(command, **params)
            conn.close()


def deleteUser(username):
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


def insertRow(username, vm_name, usernames, vm_names):
    if not (username in usernames and vm_name in vm_names):
        command = text("""
            INSERT INTO v_ms("username", "vm_name") 
            VALUES(:username, :vm_name)
            """)
        params = {'username': username,
                  'vm_name': vm_name}
        with engine.connect() as conn:
            conn.execute(command, **params)
            conn.close()


def fetchLoginActivity():
    command = text("""
        SELECT * FROM login_history
        """)
    params = {}
    with engine.connect() as conn:
        activities = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        activities.reverse()
        conn.close()
        return activities


def fetchCustomers():
    command = text("""
        SELECT * FROM customers
        """)
    params = {}
    with engine.connect() as conn:
        customers = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        conn.close()
        return customers


def insertCustomer(email, customer_id, subscription_id, location, trial_end, paid):
    command = text("""
        SELECT * FROM customers WHERE "username" = :email
        """)
    params = {'email': email}
    with engine.connect() as conn:
        customers = cleanFetchedSQL(conn.execute(command, **params).fetchall())

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


def deleteCustomer(email):
    command = text("""
        DELETE FROM customers WHERE "username" = :email 
        """)
    params = {'email': email}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def checkComputer(computer_id, username):
    command = text("""
        SELECT * FROM studios WHERE "id" = :id AND "username" = :username
        """)
    params = {'id': computer_id, 'username': username}
    with engine.connect() as conn:
        computer = cleanFetchedSQL(conn.execute(command, **params).fetchone())
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
    command = text("""
        SELECT * FROM studios WHERE "username" = :username
        """)
    params = {'username': username}
    with engine.connect() as conn:
        computers = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        out = [{'username': computer[0],
                'location': computer[1],
                'nickname': computer[2],
                'id': computer[3]} for computer in computers]
        conn.close()
        return out
    return None


def changeComputerName(username, computer_id, nickname):
    command = text("""
        UPDATE studios
        SET nickname = :nickname
        WHERE
           "username" = :username
        AND
            "id" = :id
        """)
    params = {'nickname': nickname, 'username': username, 'id': computer_id}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def fetchAllUsers():
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
    command = text("""
        SELECT * FROM users WHERE "code" = :code
        """)
    params = {'code': code}
    with engine.connect() as conn:
        user = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        return user if user else None


def changeUserCredits(username, credits):
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
    command = text("""
        INSERT INTO feedback("username", "feedback") 
        VALUES(:email, :feedback)
        """)
    params = {'email': username, 'feedback': feedback}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def updateVMIP(vm_name, ip):
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

def updateVMLocation(vm_name, location):
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
    command = text("""
        UPDATE disks SET "username" = :username WHERE "disk_name" = :disk_name
        """)
    params = {'username': username, 'disk_name': disk_name}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()

def fetchAttachableVMs(state, location):
    command = text("""
        SELECT * FROM v_ms WHERE "state" = :state AND "location" = :location AND "lock" = :lock
        """)
    params = {'state': state, 'location': location, 'lock': False}

    with engine.connect() as conn:
        vms = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        conn.close()
        return vms

def getMostRecentActivity(username):
    command = text("""
        SELECT *
        FROM login_history
        WHERE "username" = :username
        ORDER BY timestamp DESC LIMIT 1
        """)

    params = {'username': username}

    with engine.connect() as conn:
        activity = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        return activity 

def associateVMWithDisk(vm_name, disk_name):
    username = mapDiskToUser(disk_name)
    username = username if username else ''
    command = text("""
        UPDATE v_ms
        SET "disk_name" = :disk_name, "username" = :username
        WHERE
           "vm_name" = :vm_name
        """)
    params = {'vm_name': vm_name, 'disk_name': disk_name, 'username': username}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()

def lockVM(vm_name, lock):
    command = text("""
        UPDATE v_ms
        SET "lock" = :lock
        WHERE
           "vm_name" = :vm_name
        """)
    params = {'vm_name': vm_name, 'lock': lock}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()

def checkLock(vm_name):
    command = text("""
        SELECT * FROM v_ms WHERE "vm_name" = :vm_name
        """)
    params = {'vm_name': vm_name}

    with engine.connect() as conn:
        vm = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        if vm:
            return vm['lock']
        return None

def attachDiskToVM(disk_name, vm_name, lun):
    try:
        _, compute_client, _ = createClients()
        virtual_machine = getVM(vm_name)

        data_disk = compute_client.disks.get(os.getenv('VM_GROUP'), disk_name)
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
        print(e)
        return -1

def swapdisk_name(disk_name, vm_name):
    try:
        _, compute_client, _ = createClients()
        virtual_machine = getVM(vm_name)
        new_os_disk = compute_client.disks.get(os.getenv('VM_GROUP'), disk_name)

        virtual_machine.storage_profile.os_disk.managed_disk.id = new_os_disk.id
        virtual_machine.storage_profile.os_disk.name = new_os_disk.name

        print("TASK: Attaching disk " + disk_name + " to " + vm_name)
        start = time.perf_counter()
        async_disk_attach = compute_client.virtual_machines.create_or_update(
            os.getenv('VM_GROUP'), vm_name, virtual_machine
        )
        async_disk_attach.wait()
        end = time.perf_counter()
        print("SUCCESS: Disk " + disk_name + " attached to " + vm_name + " in " + str(end - start) + " seconds")

        return fractalVMStart(vm_name)
    except Exception as e:
        print("CRITICAL ERROR: " + str(e))
        return -1

def fetchAllDisks():
    command = text("""
        SELECT *
        FROM disks
        """)

    params = {}

    with engine.connect() as conn:
        disks = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        return disks

def deleteDiskFromTable(disk_name):
    command = text("""
        DELETE FROM disks WHERE "disk_name" = :disk_name 
        """)
    params = {'disk_name': disk_name}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()

def mapDiskToVM(disk_name):
    command = text("""
        SELECT * FROM v_ms WHERE "disk_name" = :disk_name
        """)

    params = {'disk_name': disk_name}

    with engine.connect() as conn:
        vms = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        return vms

def updateVM(vm_name, location, disk_name, username):
    command = text("""
        UPDATE v_ms
        SET "location" = :location, "disk_name" = :disk_name, "username" = :username
        WHERE
           "vm_name" = :vm_name
        """)
    params = {'location': location, 'vm_name': vm_name, 'disk_name': disk_name, 'username': username}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close() 


def mapDiskToUser(disk_name):
    command = text("""
        SELECT * FROM disks WHERE "disk_name" = :disk_name
        """)

    params = {'disk_name': disk_name}

    with engine.connect() as conn:
        disk = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        if disk:
            return disk['username']
        print("ERROR: No username found for disk " + disk_name)
        return None

def deleteVMFromTable(vm_name):
    command = text("""
        DELETE FROM v_ms WHERE "vm_name" = :vm_name 
        """)
    params = {'vm_name': vm_name}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()

def updateDiskState(disk_name, state):
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
    command = text("""
        UPDATE disks SET "vm_size" = :vm_size WHERE "disk_name" = :disk_name
        """)
    params = {'vm_size': vm_size, 'disk_name': disk_name}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()

def createDiskFromImageHelper(username, location, vm_size):
    disk_name = genDiskName()
    _, compute_client, _ = createClients()

    try:
        ORIGINAL_DISK = 'Fractal_Disk_Eastus'
        if location == 'southcentralus':
            ORIGINAL_DISK = 'Fractal_Disk_Southcentralus'
        elif location == 'northcentralus':
            ORIGINAL_DISK = 'Fractal_Disk_Northcentralus'

        disk_image = compute_client.disks.get('Fractal', ORIGINAL_DISK)
        print('SUCCESS: Disk found in Fractal resource pool')
        print('NOTIFICATION: Preparing to create disk {} with location {} under {} attached to a {} VM'.format(
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
        print('NOTIFICATION: Disk clone command sent. Waiting on disk to create')
        async_disk_creation.wait()
        print('SUCCESS: Disk successfully cloned to {}'.format(disk_name))
        new_disk = async_disk_creation.result()

        updateDisk(disk_name, '', location)
        assignUserToDisk(disk_name, username)
        assignVMSizeToDisk(disk_name, vm_size)

        return {'status': 200, 'disk_name': disk_name}
    except Exception as e:
        print('CRITICAL ERROR: ' + str(e))

        print("Attempting to delete the disk {}".format(disk_name))
        os_disk_delete = compute_client.disks.delete(
            os.getenv('VM_GROUP'), disk_name)
        os_disk_delete.wait()
        print("Disk {} deleted".format(disk_name))

        time.sleep(30)
        return {'status': 400, 'disk_name': None}


def sendVMStartCommand(vm_name):
    _, compute_client, _ = createClients()

    try:
        power_state = 'PowerState/deallocated'
        vm_state = compute_client.virtual_machines.instance_view(
            resource_group_name = os.getenv('VM_GROUP'), vm_name = vm_name)

        try:
            power_state = vm_state.statuses[1].code
        except Exception as e:
            print('CRITICAL ERROR: ' + str(e))
            print(vm_state.statuses)
            pass

        if not 'running' in power_state:
            print("Starting VM {}".format(vm_name))
            async_vm_start = compute_client.virtual_machines.start(
                os.environ.get('VM_GROUP'), vm_name)
            async_vm_start.wait()
            print("VM {} started".format(vm_name))
        else:
            print("Restarting VM {}".format(vm_name))
            async_vm_restart = compute_client.virtual_machines.restart(
                os.environ.get('VM_GROUP'), vm_name)
            async_vm_restart.wait()
            print("VM {} restarted",format(vm_name))

        return 1
    except Exception as e:
        print('CRITICAL ERROR: ' + str(e))
        return -1

def fractalVMStart(vm_name):
    _, compute_client, _ = createClients()

    started = False
    start_attempts = 0

    # We will try to start/restart the VM and wait for it three times in total before giving up
    while not started and start_attempts < 3:
        start_command_tries = 0

        #First, send a basic start or restart command. Try six times, if it fails, give up
        while sendVMStartCommand(vm_name) < 0 and start_command_tries < 6:
            time.sleep(10)
            start_command_tries += 1

        if start_command_tries >= 6:
            return -1

        wake_retries = 0

        # After the VM has been started/restarted, query the state. Try 12 times for the state to be running. If it is not running,
        # give up and go to the top of the while loop to send another start/restart command
        vm_state = compute_client.virtual_machines.instance_view(
            resource_group_name = os.getenv('VM_GROUP'), vm_name = vm_name)

        # Success! VM is running and ready to use
        if 'running' in vm_state.statuses[1].code:
            print('SUCCESS: Running found in status of VM {}'.format(vm_name))
            started = True
            return 1

        while not 'running' in vm_state.statuses[1].code and wake_retries < 12:
            print('VM state is currently {}, sleeping for 5 seconds and querying state again'.format(vm_state.statuses[1].code))
            time.sleep(5)
            vm_state = compute_client.virtual_machines.instance_view(
                resource_group_name = os.getenv('VM_GROUP'), vm_name = vm_name)

            # Success! VM is running and ready to use
            if 'running' in vm_state.statuses[1].code:
                print('SUCCESS: Running found in status of VM {}'.format(vm_name))
                started = True
                return 1

            wake_retries += 1

        start_attempts += 1

    return -1








