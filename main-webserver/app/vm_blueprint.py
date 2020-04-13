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
            'output': result.result
        }
        return make_response(jsonify(response), 200)
    elif result.status == 'FAILURE':
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
        vm_name = request.get_json()['vm_name']
        task = restartVM.apply_async([vm_name])
        return jsonify({'ID': task.id}), 202
    elif action == 'updateState':
        task = updateVMStates.apply_async([])
        return jsonify({'ID': task.id}), 202
    elif action == 'diskSwap':
        body = request.get_json()
        task = swapSpecificDisk.apply_async([body['disk_name'], body['vm_name']])
        return jsonify({'ID': task.id}), 202
    elif action == 'updateTable':
        task = updateVMTable.apply_async([])
        return jsonify({'ID': task.id}), 202
    elif action == 'powershell':
        body = request.get_json()
        task = runPowershell.apply_async([body['vm_name']])
        return jsonify({'ID': task.id}), 202
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
        task = createDiskFromImage.apply_async([body['username'], body['location']])
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
    elif action == 'fetch':
        body = request.get_json()
        disks = fetchUserDisks(body['username'])
        return jsonify({'disks': disks, 'status': 200}), 200
# TRACKER endpoint

@vm_bp.route('/tracker/<action>', methods = ['POST'])
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
        addTimeTable(username, 'logoff', time, is_user)
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