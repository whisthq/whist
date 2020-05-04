from .imports import *
from .tasks import *

vm_bp = Blueprint('vm_bp', __name__)

# To access jwt_required endpoints, must include
# Authorization: Bearer <Access Token JWT>
# in header of request

# To access jwt_refresh_token_required endpoints, must include
# Authorization: Bearer <Refresh Token JWT>
# in header of request


@vm_bp.route('/status/<task_id>')
def status(task_id):
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
            'output': str(result.info)
        }
        return make_response(jsonify(response), 200)
    elif result.status == 'PENDING':
        response = {
            'state': result.status,
            'output': str(result.info)
        }
        return make_response(jsonify(response), 200)
    else:
        response = {
            'state': result.status,
            'output': None
        }
        return make_response(jsonify(response), 200)

@vm_bp.route('/dummy', methods=['POST'])
def dummy():
    task = stateChangeTest.apply_async()
    return jsonify({'ID': task.id}), 202

@vm_bp.route('/vm/<action>', methods=['POST'])
def vm(action):
    if action == 'create':
        vm_size = request.get_json()['vm_size']
        location = request.get_json()['location']
        task = createVM.apply_async([vm_size, location])
        if not task:
            return jsonify({}), 400
        return jsonify({'ID': task.id}), 202
    elif action == 'fetchip':
        vm_name = request.get_json()['vm_name']
        try:
            vm = getVM(vm_name)
            ip = getIP(vm)
            return({'public_ip': ip}), 200
        except:
            return({'public_ip': None}), 404
    elif action == 'delete':
        body = request.get_json()
        vm_name, delete_disk = body['vm_name'], body['delete_disk']
        task = deleteVMResources.apply_async([vm_name, delete_disk])
        return jsonify({'ID': task.id}), 202
    elif action == 'restart':
        username = request.get_json()['username']
        vm = fetchUserVMs(username)
        if vm:
            vm_name = vm[0]['vm_name']
            task = restartVM.apply_async([vm_name])
            return jsonify({'ID': task.id}), 202
        return jsonify({'ID': None}), 404
    elif action == 'start':
        vm_name = request.get_json()['vm_name']
        task = startVM.apply_async([vm_name])
        return jsonify({'ID': task.id}), 202
    elif action == 'deallocate':
        vm_name = request.get_json()['vm_name']
        task = deallocateVM.apply_async([vm_name])
        return jsonify({'ID': task.id}), 202
    elif action == 'updateState':
        task = updateVMStates.apply_async([])
        return jsonify({'ID': task.id}), 202
    elif action == 'diskSwap':
        body = request.get_json()
        task = swapSpecificDisk.apply_async(
            [body['disk_name'], body['vm_name']])
        return jsonify({'ID': task.id}), 202
    elif action == 'updateTable':
        task = updateVMTable.apply_async([])
        return jsonify({'ID': task.id}), 202
    elif action == 'powershell':
        body = request.get_json()
        task = runPowershell.apply_async([body['vm_name']])
        return jsonify({'ID': task.id}), 202
    elif action == 'fetchall':
        body = request.get_json()
        vms = fetchUserVMs(None)
        return jsonify({'payload': vms, 'status': 200}), 200
    elif action == 'lock':
        body = request.get_json()
        lockVM(body['vm_name'], True)
        return jsonify({'status': 200}), 200
    elif action == 'unlock':
        body = request.get_json()
        lockVM(body['vm_name'], False)
        return jsonify({'status': 200}), 200
    elif action == 'connectionStatus':
        body = request.get_json()
        ready = body['ready']
        vm_ip = body['vm_ip']
        vm_info = fetchVMByIP(vm_ip)
        vm_name = vm_info['vm_name']

        updateVMState(vm_name, 'RUNNING_AVAILABLE')
        lockVM(vm_name, False)
        vmReadyToConnect(vm_name, ready)

        return jsonify({'status': 200}), 200

    return jsonify({}), 400


@vm_bp.route('/disk/<action>', methods=['POST'])
@jwt_required
def disk(action):
    if action == 'createEmpty':
        body = request.get_json()
        disk_size = body['disk_size']
        username = body['username']
        location = body['location']
        task = createEmptyDisk.apply_async([disk_size, username, location])
        if not task:
            return jsonify({}), 400
        return jsonify({'ID': task.id}), 202
    elif action == 'createFromImage':
        body = request.get_json()
        print(body)
        task = createDiskFromImage.apply_async(
            [body['username'], body['location'], body['vm_size']])
        if not task:
            return jsonify({}), 400
        return jsonify({'ID': task.id}), 202
    elif action == 'attach':
        body = request.get_json()
        task = swapDisk.apply_async([body['disk_name']])
        return jsonify({'ID': task.id}), 202
    elif action == 'detach':
        vm_name = request.get_json()['vm_name']
        disk_name = request.get_json()['disk_name']
        task = detachDisk.apply_async([vm_name, disk_name])
        if not task:
            return jsonify({}), 400
        return jsonify({'ID': task.id}), 202
    elif action == 'resync':
        task = syncDisks.apply_async([])
        return jsonify({'ID': task.id}), 202
    elif action == 'update':
        body = request.get_json()
        assignUserToDisk(body['disk_name'], body['username'])
        return jsonify({'status': 200}), 200
    elif action == 'delete':
        body = request.get_json()
        username = body['username']
        disks = fetchUserDisks(username, True)
        task_id = None
        print(disks)
        if disks:
            for disk in disks:
                task = deleteDisk.apply_async([disk['disk_name']])
                task_id = task.id
        return jsonify({'ID': task_id}), 202

# TRACKER endpoint


@vm_bp.route('/tracker/<action>', methods=['POST'])
@jwt_required
def tracker(action):
    body = request.get_json()
    time = None
    try:
        time = body['time']
    except:
        pass
    if action == 'logon':
        username = body['username']
        is_user = body['is_user']
        addTimeTable(username, 'logon', time, is_user)
    elif action == 'logoff':
        username = body['username']
        is_user = body['is_user']
        customer = fetchCustomer(username)
        if not customer:
            print(
                'CRITICAL ERROR: {} logged on/off but is not a registered customer'.format(username))
        else:
            stripe.api_key = os.getenv('STRIPE_SECRET')
            subscription_id = customer['subscription']

            try:
                payload = stripe.Subscription.retrieve(subscription_id)

                if os.getenv('HOURLY_PLAN_ID') == payload['items']['data'][0]['plan']['id']:
                    print(
                        'NOTIFICATION: {} is an hourly plan subscriber'.format(username))
                    user_activity = getMostRecentActivity(username)
                    print(user_activity)
                    if user_activity['action'] == 'logon':
                        now = dt.now()
                        logon = dt.strptime(
                            user_activity['timestamp'], '%m-%d-%Y, %H:%M:%S')
                        if now - logon > timedelta(minutes=0):
                            amount = round(
                                79 * (now - logon).total_seconds()/60/60)
                            addPendingCharge(username, amount)
                    else:
                        print(
                            'CRITICAL ERROR: {} logged off but no log on was recorded')
            except:
                pass

        addTimeTable(username, 'logoff', time, is_user)
    elif action == 'startup':
        username = body['username']
        is_user = body['is_user']
        addTimeTable(username, 'startup', time, is_user)
    elif action == 'clear':
        deleteTimeTable()
    elif action == 'fetch':
        activity = fetchLoginActivity()
        return jsonify({'payload': activity}), 200
    elif action == 'fetchMostRecent':
        activity = getMostRecentActivity(body['username'])
        return jsonify({'payload': activity})
    return jsonify({}), 200

# INFO endpoint


@vm_bp.route('/info/<action>', methods=['GET', 'POST'])
@jwt_required
def info(action):
    body = request.get_json()
    if action == 'list_all' and request.method == 'GET':
        task = fetchAll.apply_async([False])
        if not task:
            return jsonify({}), 400
        return jsonify({'ID': task.id}), 202
    if action == 'list_all_disks' and request.method == 'GET':
        disks = fetchUserDisks(None)
        return jsonify({'disks': disks}), 200
    if action == 'update_db' and request.method == 'POST':
        task = fetchAll.apply_async([True])
        if not task:
            return jsonify({}), 400
        return jsonify({'ID': task.id}), 202

    return jsonify({}), 400


@vm_bp.route('/logs', methods=['POST'])
def logs():
    body = request.get_json()
    vm_ip = None
    if 'vm_ip' in body:
        vm_ip = body['vm_ip']

    task = storeLogs.apply_async(
        [body['sender'], body['connection_id'], body['logs'], vm_ip])
    return jsonify({'ID': task.id}), 202


@vm_bp.route('/logs/fetch', methods=['POST'])
def fetch_logs():
    body = request.get_json()
    task = fetchLogs.apply_async([body['username']])
    return jsonify({'ID': task.id}), 202

