from .imports import *
from .tasks import *
from app import *

vm_bp = Blueprint('vm_bp', __name__)

# To access jwt_required endpoints, must include
# Authorization: Bearer <Access Token JWT>
# in header of request

# To access jwt_refresh_token_required endpoints, must include
# Authorization: Bearer <Refresh Token JWT>
# in header of request


@vm_bp.route('/status/<task_id>')
@generateID
def status(task_id, **kwargs):
    sendInfo(kwargs['ID'], 'GET request sent to /status/{}'.format(task_id), papertrail = False)

    result = celery.AsyncResult(task_id)
    if result.status == 'SUCCESS':
        response = {
            'state': result.status,
            'output': result.result,
        }
        return make_response(jsonify(response), 200)
    elif result.status == 'FAILURE':
        response = {
            'state': result.status,
            'output': result.info
        }
        return make_response(jsonify(response), 200)
    elif result.status == 'PENDING':
        response = {
            'state': result.status,
            'output': result.info
        }
        return make_response(jsonify(response), 200)
    else:
        response = {
            'state': result.status,
            'output': None
        }
        return make_response(jsonify(response), 200)


@vm_bp.route('/vm/<action>', methods=['POST'])
@generateID
def vm(action, **kwargs):
    if action == 'create':
        sendInfo(kwargs['ID'], 'POST request sent to /vm/create')

        body = request.get_json()
        vm_size = body['vm_size']
        location = body['location']
        operating_system = 'Windows'

        if 'operating_system' in body.keys():
            operating_system = body['operating_system']

        sendInfo(kwargs['ID'], 'Creating VM of size {}, location {}, operating system {}'.format(vm_size, location, operating_system))

        task = createVM.apply_async([vm_size, location, operating_system])
        if not task:
            return jsonify({}), 400
        return jsonify({'ID': task.id}), 202
    elif action == 'fetchip':
        sendInfo(kwargs['ID'], 'POST request sent to /vm/fetchip')

        vm_name = request.get_json()['vm_name']
        try:
            vm = getVM(vm_name)
            ip = getIP(vm)

            return({'public_ip': ip}), 200
        except:
            return({'public_ip': None}), 404
    elif action == 'delete':
        sendInfo(kwargs['ID'], 'POST request sent to /vm/delete')

        body = request.get_json()
        vm_name, delete_disk = body['vm_name'], body['delete_disk']
        task = deleteVMResources.apply_async([vm_name, delete_disk])
        return jsonify({'ID': task.id}), 202
    elif action == 'restart':
        sendInfo(kwargs['ID'], 'POST request sent to /vm/restart')

        username = request.get_json()['username']
        vm = fetchUserVMs(username)
        if vm:
            vm_name = vm[0]['vm_name']
            task = restartVM.apply_async([vm_name])
            return jsonify({'ID': task.id}), 202
        return jsonify({'ID': None}), 404
    elif action == 'start':
        sendInfo(kwargs['ID'], 'POST request sent to /vm/start')

        vm_name = request.get_json()['vm_name']
        task = startVM.apply_async([vm_name])
        return jsonify({'ID': task.id}), 202
    elif action == 'deallocate':
        sendInfo(kwargs['ID'], 'POST request sent to /vm/deallocate')

        vm_name = request.get_json()['vm_name']
        task = deallocateVM.apply_async([vm_name])
        return jsonify({'ID': task.id}), 202
    elif action == 'updateState':
        sendInfo(kwargs['ID'], 'POST request sent to /vm/updateState')

        task = updateVMStates.apply_async([])
        return jsonify({'ID': task.id}), 202
    elif action == 'diskSwap':
        sendInfo(kwargs['ID'], 'POST request sent to /vm/diskSwap')

        body = request.get_json()
        task = swapSpecificDisk.apply_async(
            [body['disk_name'], body['vm_name']])
        return jsonify({'ID': task.id}), 202
    elif action == 'updateTable':
        sendInfo(kwargs['ID'], 'POST request sent to /vm/updateTable')

        task = updateVMTable.apply_async([])
        return jsonify({'ID': task.id}), 202
    elif action == 'fetchall':
        sendInfo(kwargs['ID'], 'POST request sent to /vm/fetchall')

        body = request.get_json()
        vms = fetchUserVMs(None)
        return jsonify({'payload': vms, 'status': 200}), 200
    elif action == 'winlogonStatus':
        sendInfo(kwargs['ID'], 'POST request sent to /vm/winlogonStatus', papertrail = False)

        body = request.get_json()
        ready = body['ready']
        vm_ip = None
        if request.headers.getlist('X-Forwarded-For'):
            vm_ip = request.headers.getlist('X-Forwarded-For')[0]
        else:
            vm_ip = request.remote_addr

        vm_info = fetchVMByIP(vm_ip)

        if vm_info:
            vm_name = vm_info['vm_name']

            updateVMState(vm_name, 'RUNNING_AVAILABLE')
            lockVM(vm_name, False)
            vmReadyToConnect(vm_name, ready)
        else:
            sendError(kwargs['ID'], 'No VM found for IP {}'.format(str(vm_ip)))

        return jsonify({'status': 200}), 200
    elif action == 'connectionStatus':
        sendInfo(kwargs['ID'], 'POST request sent to /vm/connectionStatus', papertrail = False)

        body = request.get_json()
        available = body['available']

        vm_ip = None
        if request.headers.getlist('X-Forwarded-For'):
            vm_ip = request.headers.getlist('X-Forwarded-For')[0]
        else:
            vm_ip = request.remote_addr

        vm_info = fetchVMByIP(vm_ip)
        if vm_info:
            vm_name = vm_info['vm_name']

            if available:
                updateVMState(vm_name, 'RUNNING_AVAILABLE')
                lockVM(vm_name, False)
            else:
                updateVMState(vm_name, 'RUNNING_UNAVAILABLE')
                lockVM(vm_name, True)
        else:
            sendError(kwargs['ID'], 'No VM found for IP {}'.format(str(vm_ip)))

        return jsonify({'status': 200}), 200
    return jsonify({}), 400


@vm_bp.route('/disk/<action>', methods=['POST'])
@jwt_required
@generateID
def disk(action, **kwargs):
    if action == 'createEmpty':
        sendInfo(kwargs['ID'], 'POST request sent to /disk/createEmpty')

        body = request.get_json()
        disk_size = body['disk_size']
        username = body['username']
        location = body['location']
        task = createEmptyDisk.apply_async([disk_size, username, location])
        if not task:
            return jsonify({}), 400
        return jsonify({'ID': task.id}), 202
    elif action == 'createFromImage':
        sendInfo(kwargs['ID'], 'POST request sent to /disk/createFromImage')

        body = request.get_json()

        task = createDiskFromImage.apply_async(
            [body['username'], body['location'], body['vm_size']])
        if not task:
            return jsonify({}), 400
        return jsonify({'ID': task.id}), 202
    elif action == 'attach':
        sendInfo(kwargs['ID'], 'POST request sent to /disk/attach')

        body = request.get_json()
        task = swapDisk.apply_async([kwargs['ID'], body['disk_name']])
        return jsonify({'ID': task.id}), 202
    elif action == 'detach':
        sendInfo(kwargs['ID'], 'POST request sent to /disk/detach')

        vm_name = request.get_json()['vm_name']
        disk_name = request.get_json()['disk_name']
        task = detachDisk.apply_async([vm_name, disk_name])
        if not task:
            return jsonify({}), 400
        return jsonify({'ID': task.id}), 202
    elif action == 'resync':
        sendInfo(kwargs['ID'], 'POST request sent to /disk/resync')

        task = syncDisks.apply_async([])
        return jsonify({'ID': task.id}), 202
    elif action == 'update':
        sendInfo(kwargs['ID'], 'POST request sent to /disk/update')

        body = request.get_json()
        assignUserToDisk(body['disk_name'], body['username'])
        return jsonify({'status': 200}), 200
    elif action == 'delete':
        sendInfo(kwargs['ID'], 'POST request sent to /disk/delete')

        body = request.get_json()
        username = body['username']
        disks = fetchUserDisks(username, True)
        task_id = None

        if disks:
            for disk in disks:
                task = deleteDisk.apply_async([disk['disk_name']])
                task_id = task.id
        return jsonify({'ID': task_id}), 202

# TRACKER endpoint


@vm_bp.route('/tracker/<action>', methods=['POST'])
@jwt_required
@generateID
def tracker(action, **kwargs):
    body = request.get_json()
    time = None
    try:
        time = body['time']
    except:
        pass
    if action == 'logon':
        sendInfo(kwargs['ID'], 'POST request sent to /tracker/logon')

        username = body['username']
        is_user = body['is_user']
        addTimeTable(username, 'logon', time, is_user)
    elif action == 'logoff':
        sendInfo(kwargs['ID'], 'POST request sent to /tracker/logoff')

        username = body['username']
        is_user = body['is_user']
        customer = fetchCustomer(username)
        if not customer:
            sendCritical(kwargs['ID'], '{} logged on/off but is not a registered customer'.format(username))
        else:
            stripe.api_key = os.getenv('STRIPE_SECRET')
            subscription_id = customer['subscription']

            try:
                payload = stripe.Subscription.retrieve(subscription_id)

                if os.getenv('HOURLY_PLAN_ID') == payload['items']['data'][0]['plan']['id']:
                    sendInfo(kwargs['ID'], '{} is an hourly plan subscriber'.format(username))
                    user_activity = getMostRecentActivity(username)
                    if user_activity['action'] == 'logon':
                        now = dt.now()
                        logon = dt.strptime(
                            user_activity['timestamp'], '%m-%d-%Y, %H:%M:%S')
                        if now - logon > timedelta(minutes=0):
                            amount = round(
                                79 * (now - logon).total_seconds()/60/60)
                            addPendingCharge(username, amount)
                    else:
                        sendError(kwargs['ID'], '{} logged off but no logon was recorded'.format(username))
            except:
                pass

        addTimeTable(username, 'logoff', time, is_user)
    elif action == 'startup':
        sendInfo(kwargs['ID'], 'POST request sent to /tracker/startup')

        username = body['username']
        is_user = body['is_user']
        addTimeTable(username, 'startup', time, is_user)
    elif action == 'fetch':
        sendInfo(kwargs['ID'], 'POST request sent to /tracker/fetch')

        activity = fetchLoginActivity()
        return jsonify({'payload': activity}), 200
    elif action == 'fetchMostRecent':
        sendInfo(kwargs['ID'], 'POST request sent to /tracker/fetchMostRecent')

        activity = getMostRecentActivity(body['username'])
        return jsonify({'payload': activity})
    return jsonify({}), 200

# INFO endpoint


@vm_bp.route('/info/<action>', methods=['GET', 'POST'])
@jwt_required
@generateID
def info(action, **kwargs):
    body = request.get_json()
    if action == 'list_all' and request.method == 'GET':
        sendInfo(kwargs['ID'], 'GET request sent to /info/list_all')

        task = fetchAll.apply_async([False])
        if not task:
            return jsonify({}), 400
        return jsonify({'ID': task.id}), 202
    if action == 'list_all_disks' and request.method == 'GET':
        sendInfo(kwargs['ID'], 'GET request sent to /info/list_all_disks')

        disks = fetchUserDisks(None)
        return jsonify({'disks': disks}), 200
    if action == 'update_db' and request.method == 'POST':
        sendInfo(kwargs['ID'], 'POST request sent to /info/update_db')

        task = fetchAll.apply_async([True])
        if not task:
            return jsonify({}), 400
        return jsonify({'ID': task.id}), 202

    return jsonify({}), 400


@vm_bp.route('/logs', methods=['POST'])
@generateID
def logs(**kwargs):
    sendInfo(kwargs['ID'], 'POST request sent to /logs')

    body = request.get_json()
    vm_ip = None
    if 'vm_ip' in body:
        vm_ip = body['vm_ip']

    task = storeLogs.apply_async(
        [body['sender'], body['connection_id'], body['logs'], vm_ip])
    return jsonify({'ID': task.id}), 202


@vm_bp.route('/logs/fetch', methods=['POST'])
@generateID
def fetch_logs(**kwargs):
    sendInfo(kwargs['ID'], 'POST request sent to /logs/fetch')

    body = request.get_json()
    task = fetchLogs.apply_async([body['username']])
    return jsonify({'ID': task.id}), 202
