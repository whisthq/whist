from .utils import *

engine = db.create_engine(
    os.getenv('DATABASE_URL'), echo=True)
conn = engine.connect()
metadata = db.MetaData()


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

def createNic(vnetName, subnetName, ipName, nicName, tries):
    _, _, network_client = createClients()
    if tries == 0:
        oldVnets = [cell[0] for cell in list(conn.execute('SELECT "vnetName" FROM v_nets'))]
        oldSubnets = [cell[0] for cell in list(conn.execute('SELECT "subnetName" FROM v_nets'))]
        oldIPs = [cell[0] for cell in list(conn.execute('SELECT "ipConfigName" FROM v_nets'))]
        oldNics = [cell[0] for cell in list(conn.execute('SELECT "nicName" FROM v_nets'))]
        vnetName, subnetName, ipName, nicName = genHaiku(4)
        while (vnetName in oldVnets) or (subnetName in oldSubnets) or (ipName in oldIPs) or (nicName in oldNics):
             vnetName, subnetName, ipName, nicName = genHaiku(4)
        command = text("""
            INSERT INTO v_nets("vnetName", "subnetName", "ipConfigName", "nicName") 
            VALUES(:vnetName, :subnetName, :ipConfigName, :nicName)
            """)
        params = {'vnetName': vnetName, 'subnetName': subnetName, 'ipConfigName': ipName, 'nicName': nicName}
        conn.execute(command, **params)
    try:
        async_vnet_creation = network_client.virtual_networks.create_or_update(
            os.getenv('VM_GROUP'),
            vnetName,
            {
                'location': os.getenv('LOCATION'),
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
            'location': os.getenv('LOCATION'),
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
                'location': os.getenv('LOCATION'),
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
            time.sleep(2)
            createNic(vnetName, subnetName, ipName, nicName, tries + 1)
        else: return None

def createVMParameters(nic_id, vm_size):
    oldVMs = [cell[0] for cell in list(conn.execute('SELECT "vmName" FROM v_ms'))]
    oldUserNames = [cell[0] for cell in list(conn.execute('SELECT "vmUserName" FROM v_ms'))]
    vmName, userName = genHaiku(2)
    while (vmName in oldVMs) or (userName in oldUserNames):
         vmName, userName = genHaiku(2)

    vm_reference = {
        'publisher': 'MicrosoftWindowsServer',
        'offer': 'WindowsServer',
        'sku': '2016-Datacenter',
        'version': 'latest'
    }

    pwd = generatePassword()
    pwd_token = jwt.encode({'pwd': pwd}, os.getenv('SECRET_KEY'))

    command = text("""
        INSERT INTO v_ms("vmName", "vmPassword", "vmUserName", "osDisk", "running") 
        VALUES(:vmName, :vmPassword, :vmUserName, :osDisk, :running)
        """)
    params = {'vmName': vmName, 'vmPassword': pwd_token, 'vmUserName': userName, 'osDisk': None, 'running': False}
    conn.execute(command, **params)

    return {'params': {
        'location': 'eastus',
        'os_profile': {
            'computer_name': vmName,
            'admin_username': userName,
            'admin_password': pwd
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
        },
        'network_profile': {
            'network_interfaces': [{
                'id': nic_id,
            }]
        },
    }, 'vmName': vmName}

def getVM(vm_name):
    _, compute_client, _= createClients()
    virtual_machine = compute_client.virtual_machines.get(
        os.environ.get('VM_GROUP'),
        vm_name
    )
    return virtual_machine

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

# def validate(username, password):
#     command = text("""
#         SELECT "userName"
#         FROM users
#         WHERE "userName" = :user""")
#     params = {'user': username}
#     conn.execute(command, **params)

def registerUser(username, vm_name):
    command = text("""
        INSERT INTO users("userName", "currentVM") 
        VALUES(:userName, :currentVM)
        """)
    params = {'userName': username, 'currentVM': vm_name}
    conn.execute(command, **params)

def loginUser(username):
    command = text("""
        SELECT * FROM users WHERE "userName" = :userName
        """)
    params = {'userName': username}
    user = conn.execute(command, **params).fetchall()
    if len(user) > 0:
        return user[0][1]
    return None

def fetchVMCredentials(vm_name):
    command = text("""
        SELECT * FROM v_ms WHERE "vmName" = :vm_name
        """)
    params = {'vm_name': vm_name}
    vm_info = conn.execute(command, **params).fetchall()[0]
    vm_name, username, password = vm_info[0], vm_info[2], vm_info[1]
    # Get public IP address
    vm = getVM(vm_name)
    ip = getIP(vm)
    # Decode password
    password = jwt.decode(password, os.getenv('SECRET_KEY'))
    return {'username': username,
            'password': password['pwd'],
            'public_ip': ip}
