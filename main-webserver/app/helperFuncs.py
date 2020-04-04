from .utils import *
from app import engine

def createClients():
    subscription_id = os.getenv('AZURE_SUBSCRIPTION_ID')
    credentials = ServicePrincipalCredentials(
        client_id = os.getenv('AZURE_CLIENT_ID'),
        secret = os.getenv('AZURE_CLIENT_SECRET'),
        tenant = os.getenv('AZURE_TENANT_ID')
    )
    r = ResourceManagementClient(credentials, subscription_id)
    c = ComputeManagementClient(credentials, subscription_id)
    n = NetworkManagementClient(credentials, subscription_id)
    return r, c, n

def createNic(name, location, tries):
    _, _, network_client = createClients()
    vnetName, subnetName, ipName, nicName = name + '_vnet', name + '_subnet', name + '_ip', name + '_nic'
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

        #Create Subnet
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

        command = text("""
            INSERT INTO v_nets("vnetName", "subnetName", "ipConfigName", "nicName") 
            VALUES(:vnetName, :subnetName, :ipConfigName, :nicName)
            """)
        params = {'vnetName': vnetName, 'subnetName': subnetName, 'ipConfigName': ipName, 'nicName': nicName}
        with engine.connect() as conn:
            conn.execute(command, **params)
        return async_nic_creation.result()
    except Exception as e:
        if tries < 5:
            print(e)
            time.sleep(3)
            return createNic(name, location, tries + 1)
        else: return None

def deleteResource(name):
    _, compute_client, network_client = createClients()
    vnetName, subnetName, ipName, nicName = name + '_vnet', name + '_subnet', name + '_ip', name + '_nic'
    hr = 1

    try:
        print("Deallocating VM...")
        async_vm_deallocate = compute_client.virtual_machines.deallocate(
            os.getenv('VM_GROUP'), name)
        async_vm_deallocate.wait()

        async_vm_delete = compute_client.virtual_machines.delete(
            os.getenv('VM_GROUP'), name)
        async_vm_delete.wait()
        print("VM deleted")
    except Exception as e:
        print(e)
        hr = -1

    try:
        async_vnet_delete = network_client.virtual_networks.delete(
            os.getenv('VM_GROUP'),
            vnetName
        )
        async_vnet_delete.wait()
        print("Vnet deleted")
    except Exception as e:
        print(e)
        hr = -1

    try:
        async_subnet_delete = network_client.subnets.delete(
            os.getenv('VM_GROUP'),
            vnetName,
            subnetName
        )
        async_subnet_delete.wait()
        print("Subnet deleted")
    except Exception as e:
        print(e)
        hr = -1

    try:
        async_ip_delete = network_client.public_ip_addresses.delete(
            os.getenv('VM_GROUP'),
            ipName
        )
        async_ip_delete.wait()
        print("IP deleted")
    except Exception as e:
        print(e)
        hr = -1

    try:
        async_nic_delete = network_client.network_interfaces.delete(
            os.getenv('VM_GROUP'),
            nicName
        )
        async_nic_delete.wait()
        print("NIC deleted")
    except Exception as e:
        print(e)
        hr = -1

    # virtual_machine = getVM(name)
    # os_disk_name = virtual_machine.storage_profile.os_disk.name
    # os_disk = compute_client.disks.get(os.getenv('VM_GROUP'), os_disk_name)

    return hr


def createVMParameters(vmName, nic_id, vm_size, location):
    with engine.connect() as conn:
        oldUserNames = [cell[0] for cell in list(conn.execute('SELECT "vmUserName" FROM v_ms'))]
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
            INSERT INTO v_ms("vmName", "vmUserName", "osDisk") 
            VALUES(:vmName, :vmUserName, :osDisk)
            """)
        params = {'vmName': vmName, 'vmUserName': userName, 'osDisk': None}
        with engine.connect() as conn:
            conn.execute(command, **params)
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
                    }
                },
                'network_profile': {
                    'network_interfaces': [{
                        'id': nic_id,
                    }]
                },
            }, 'vmName': vmName}

def getVM(vm_name):
    _, compute_client, _= createClients()
    try:
        virtual_machine = compute_client.virtual_machines.get(
            os.environ.get('VM_GROUP'),
            vm_name
        )
        return virtual_machine
    except: return None

def singleValueQuery(value):
    command = text("""
        SELECT * FROM v_ms WHERE "vmName" = :value
        """)
    params = {'value': value}
    with engine.connect() as conn:
        exists = conn.execute(command, **params).fetchall()
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


def registerUserVM(username, vm_name):
    command = text("""
        UPDATE v_ms
        SET username = :username
        WHERE
           "vmName" = :vm_name
        """)
    params = {'username': username, 'vm_name': vm_name}
    with engine.connect() as conn:
        conn.execute(command, **params)

def loginUserVM(username):
    command = text("""
        SELECT * FROM v_ms WHERE "vmUserName" = :username
        """)
    params = {'username': username}
    with engine.connect() as conn:
        user = conn.execute(command, **params).fetchall()
        if len(user) > 0:
            return user[0][0]
    return None

def loginUser(username, password):
    if password != os.getenv('ADMIN_PASSWORD'):
        command = text("""
            SELECT * FROM users WHERE "userName" = :userName AND "password" = :password
            """)
        pwd_token = jwt.encode({'pwd': password}, os.getenv('SECRET_KEY'))
        params = {'userName': username, 'password': pwd_token}
        with engine.connect() as conn:
            user = conn.execute(command, **params).fetchall()
            return len(user) > 0
    else:
        command = text("""
            SELECT * FROM users WHERE "userName" = :userName
            """)
        params = {'userName': username}
        with engine.connect() as conn:
            user = conn.execute(command, **params).fetchall()
            return len(user) > 0

def lookup(username):
    command = text("""
        SELECT * FROM users WHERE "userName" = :userName
        """)
    params = {'userName': username}
    with engine.connect() as conn:
        user = conn.execute(command, **params).fetchall()
        return len(user) > 0

def genUniqueCode():
    with engine.connect() as conn:
        old_codes = [cell[0] for cell in list(conn.execute('SELECT "code" FROM users'))]
        new_code = generateCode()
        while new_code in old_codes:
             new_code = generateCode()
        return new_code


def registerUser(username, password, token):
    pwd_token = jwt.encode({'pwd': password}, os.getenv('SECRET_KEY'))
    code = genUniqueCode()
    command = text("""
        INSERT INTO users("userName", "password", "code", "id") 
        VALUES(:userName, :password, :code, :token)
        """)
    params = {'userName': username, 'password': pwd_token, 'code': code, 'token': token}
    with engine.connect() as conn:
        try:
            conn.execute(command, **params)
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
                WHERE "userName" = :userName
                """)            
            params = {'code': code, 'userName': row[0]}
            conn.execute(command, **params)

def generateIDs():
    with engine.connect() as conn:
        for row in list(conn.execute('SELECT * FROM users')):
            token = generateToken(row[0])
            print("THE USER IS " + row[0])
            print("THE TOKEN IS " + token)
            command = text("""
                UPDATE users 
                SET "id" = :token
                WHERE "userName" = :userName
                """)            
            params = {'token': token, 'userName': row[0]}
            conn.execute(command, **params)


def resetPassword(username, password):
    pwd_token = jwt.encode({'pwd': password}, os.getenv('SECRET_KEY'))
    command = text("""
        UPDATE users 
        SET "password" = :password
        WHERE "userName" = :userName
        """)
    params = {'userName': username, 'password': pwd_token}
    with engine.connect() as conn:
        conn.execute(command, **params)

def resetVMCredentials(username, vm_name):
    command = text("""
        UPDATE v_ms
        SET "vmUserName" = :userName
        WHERE "vmName" = :vm_name
        """)
    params = {'userName': username, 'vm_name': vm_name}
    with engine.connect() as conn:
        conn.execute(command, **params)


def fetchVMCredentials(vm_name):
    command = text("""
        SELECT * FROM v_ms WHERE "vmName" = :vm_name
        """)
    params = {'vm_name': vm_name}
    with engine.connect() as conn:
        vm_info = conn.execute(command, **params).fetchone()
        vm_name, username = vm_info[0], vm_info[1]
        # Get public IP address
        vm = getVM(vm_name)
        ip = getIP(vm)
        # Decode password
        return {'username': username,
                'vm_name': vm_name,
                'public_ip': ip}

def genVMName():
    with engine.connect() as conn:
        oldVMs = [cell[0] for cell in list(conn.execute('SELECT "vmName" FROM v_ms'))]
        vmName = genHaiku(1)[0]
        while vmName in oldVMs:
             vmName = genHaiku(1)[0]
        return vmName

def storeForm(name, email, cubeType):
    command = text("""
        INSERT INTO form("fullname", "email", "cubetype") 
        VALUES(:name, :email, :cubeType)
        """)
    params = {'name': name, 'email': email, 'cubeType': cubeType}
    with engine.connect() as conn:
        conn.execute(command, **params)

def storePreOrder(address1, address2, zipCode, email, order):
    command = text("""
        INSERT INTO pre_order("address1", "address2", "zipcode", "email", "base", "enhanced", "power") 
        VALUES(:address1, :address2, :zipcode, :email, :base, :enhanced, :power)
        """)
    params = {'address1': address1, 'address2': address2, 'zipcode': zipCode, 'email': email, 
              'base': int(order['base']), 'enhanced': int(order['enhanced']), 'power': int(order['power'])}
    with engine.connect() as conn:
        conn.execute(command, **params)

def addTimeTable(username, action, time):
    command = text("""
        INSERT INTO login_history("username", "timestamp", "action") 
        VALUES(:userName, :currentTime, :action)
        """)
    
    params = {'userName': username, 'currentTime': dt.now().strftime('%m-%d-%Y, %H:%M:%S'), 'action': action}

    with engine.connect() as conn:
        conn.execute(command, **params)

def deleteTimeTable():
    command = text("""
        DELETE FROM login_history
        """)
    params = {}

    with engine.connect() as conn:
        conn.execute(command, **params)


def fetchUserVMs(username):
    if(username):
        command = text("""
            SELECT * FROM v_ms WHERE "vmUserName" = :username
            """)
        params = {'username': username}
        with engine.connect() as conn:
            vms_info = conn.execute(command, **params).fetchall()
            out = [{'vm_name': vm_info[0], 'vm_username': vm_info[1]} for vm_info in vms_info]
            return out
    else:
        command = text("""
            SELECT * FROM v_ms
            """)
        params = {}
        with engine.connect() as conn:
            vms_info = conn.execute(command, **params).fetchall()
            out = {vm_info[0]: {'username': vm_info[1], 'ip': vm_info[3]} for vm_info in vms_info}
            return out

def fetchUserCode(username):
    try:
        command = text("""
            SELECT * FROM users WHERE "userName" = :userName
            """)
        params = {'userName': username}
        with engine.connect() as conn:
            user = conn.execute(command, **params).fetchone()
            return user[2]
    except:
        return None

def deleteRow(username, vm_name, usernames, vm_names):
    if not (vm_name in vm_names):
        command = text("""
            DELETE FROM v_ms WHERE "vmName" = :vm_name 
            """)
        params = {'vm_name': vm_name}
        with engine.connect() as conn:
            conn.execute(command, **params)

def deleteUser(username):
    command = text("""
        DELETE FROM users WHERE "userName" = :username 
        """)
    params = {'username': username}
    with engine.connect() as conn:
        try:
            conn.execute(command, **params)
            return 200
        except:
            return 404

def insertRow(username, vm_name, usernames, vm_names):
    if not (username in usernames and vm_name in vm_names):
        command = text("""
            INSERT INTO v_ms("vmUserName", "vmName") 
            VALUES(:username, :vm_name)
            """)
        params = {'username': username, 
                  'vm_name': vm_name}
        with engine.connect() as conn:
            conn.execute(command, **params)

def fetchLoginActivity():
    command = text("""
        SELECT * FROM login_history
        """)
    params = {}
    with engine.connect() as conn:
        activities = conn.execute(command, **params).fetchall()
        out = [{'username': activity[0], 
                'timestamp': activity[1], 
                'action': activity[2]} for activity in activities]
        out.reverse()
        return out

def fetchCustomers():
    command = text("""
        SELECT * FROM customers
        """)
    params = {}
    with engine.connect() as conn:
        customers = conn.execute(command, **params).fetchall()
        out = [{'email': customer[0], 
                'id': customer[1],
                'subscription': customer[2]} for customer in customers]
        return out

def insertCustomer(email, customer_id, subscription_id, location):
    command = text("""
        INSERT INTO customers("email", "id", "subscription", "location") 
        VALUES(:email, :id, :subscription, :location)
        """)

    params = {'email': email, 
              'id': customer_id,
              'subscription': subscription_id,
              'location': location}

    with engine.connect() as conn:
        conn.execute(command, **params)

def deleteCustomer(email):
    command = text("""
        DELETE FROM customers WHERE "email" = :email 
        """)
    params = {'email': email}
    with engine.connect() as conn:
        conn.execute(command, **params)

def checkComputer(computer_id, username):
    command = text("""
        SELECT * FROM studios WHERE "id" = :id AND "username" = :username
        """)
    params = {'id': computer_id, 'username': username}
    with engine.connect() as conn:
        computer = conn.execute(command, **params).fetchone()
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

def fetchComputers(username):
    command = text("""
        SELECT * FROM studios WHERE "username" = :username
        """)
    params = {'username': username}
    with engine.connect() as conn:
        computers = conn.execute(command, **params).fetchall()
        out = [{'username': computer[0], 
                'location': computer[1],
                'nickname': computer[2],
                'id': computer[3]} for computer in computers]
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

def fetchAllUsers():
    command = text("""
        SELECT * FROM users
        """)
    params = {}
    with engine.connect() as conn:
        users = conn.execute(command, **params).fetchall()
        out = [{'username': user[0],
               'code': user[2],
               'creditsOutstanding': user[3],
               'verified': user[4]} for user in users]
        return out
    return None

def mapCodeToUser(code):
    command = text("""
        SELECT * FROM users WHERE "code" = :code
        """)
    params = {'code': code}
    with engine.connect() as conn:
        user = conn.execute(command, **params).fetchone()
        if user:
            return {'email': user[0], 'creditsOutstanding': user[3]}
    return None

def changeUserCredits(username, credits):
    command = text("""
        UPDATE users
        SET "creditsOutstanding" = :credits
        WHERE
           "userName" = :username
        """)
    params = {'credits' : credits, 'username': username}
    with engine.connect() as conn:
        conn.execute(command, **params)

def getUserCredits(username):
    command = text("""
        SELECT * FROM users
        WHERE "userName" = :username
        """)
    params = {'username': username}
    with engine.connect() as conn:
        users = conn.execute(command, **params).fetchone()
        if users:
            return users[3]
    return 0

def fetchCodes():
    command = text("""
        SELECT * FROM users
        """)
    params = {}
    with engine.connect() as conn:
        users = conn.execute(command, **params).fetchall()
        return [user[2] for user in users]
    return None

def userVMStatus(username):
    has_paid = False
    has_vm = False

    command = text("""
        SELECT * FROM customers
        WHERE "email" = :username
        """)
    params = {'username': username}
    with engine.connect() as conn:
        user = conn.execute(command, **params).fetchone()
        if user:
            has_paid = True

    command = text("""
        SELECT * FROM v_ms
        WHERE "vmUserName" = :username
        """)
    params = {'username': username}
    with engine.connect() as conn:
        user = conn.execute(command, **params).fetchone()
        if user:
            has_vm = True

    if not has_paid and not has_vm:
        return 'not_created'

    if has_paid and not has_vm:
        return 'is_creating'

    if has_paid and has_vm:
        return 'has_created'

    return 'has_not_paid'

def checkUserVerified(username):
    command = text("""
        SELECT * FROM users WHERE "userName" = :userName
        """)
    params = {'userName': username}
    with engine.connect() as conn:
        user = conn.execute(command, **params).fetchone()
        if user:
            return user[4]
        return False

def fetchUserToken(username):
    command = text("""
        SELECT * FROM users WHERE "userName" = :userName
        """)
    params = {'userName': username}
    with engine.connect() as conn:
        user = conn.execute(command, **params).fetchone()
        if user:
            return user[5]
        return None

def makeUserVerified(username, verified):
    command = text("""
        UPDATE users
        SET verified = :verified
        WHERE
           "userName" = :username
        """)
    params = {'verified': verified, 'username': username}
    with engine.connect() as conn:
        conn.execute(command, **params)

def storeFeedback(username, feedback):
    command = text("""
        INSERT INTO feedback("email", "feedback") 
        VALUES(:email, :feedback)
        """)
    params = {'email': username, 'feedback': feedback}
    with engine.connect() as conn:
        conn.execute(command, **params)

def updateVMIP(vm_name, ip):
    command = text("""
        UPDATE v_ms
        SET ip = :ip
        WHERE
           "vmName" = :vm_name
        """)
    params = {'ip': ip, 'vm_name': vm_name}
    with engine.connect() as conn:
        conn.execute(command, **params)