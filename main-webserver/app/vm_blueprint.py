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
@jwt_required
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

# VM endpoint

@vm_bp.route('/vm/create', methods = ['POST'])
@jwt_required
def vm_create():
    body = request.get_json()
    vm_size = body['vm_size']
    location = body['location']
    task = createVM.apply_async([vm_size, location])
    if not task: 
        return jsonify({}), 400
    return jsonify({'ID': task.id}), 202

@vm_bp.route('/vm/fetchip', methods = ['POST'])
@jwt_required
def vm_fetch_ip():
    body = request.get_json()
    vm_name = body['vm_name']
    try:
        vm = getVM(vm_name)
        ip = getIP(vm)
        return({'public_ip': ip}), 200
    except:
        return({'public_ip': None}), 404
    
@vm_bp.route('/vm/delete', methods = ['POST'])
@jwt_required
def vm_delete():
    body = request.get_json()
    vm_name = body['vm_name']
    task = deleteVMResources.apply_async([vm_name])
    return jsonify({'ID': task.id}), 202


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
        addTimeTable(username, 'logon', time)
    elif action == 'logoff':
        username = body['username']
        addTimeTable(username, 'logoff', time)
    elif action == 'clear':
        deleteTimeTable()
    elif action == 'fetch':
        activity = fetchLoginActivity()
        return jsonify({'payload': activity}), 200
    return jsonify({}), 200

# INFO endpoint

@vm_bp.route('/info/list_all', methods = ['GET'])
@jwt_required
def info_list_all():
    task = fetchAll.apply_async([False]) # False = do not update db
    if not task:
        return jsonify({}), 400
    return jsonify({'ID': task.id}), 202

@vm_bp.route('/info/update_db', methods = ['POST'])
@jwt_required
def info_update_db():
    task = fetchAll.apply_async([True]) # True = update db
    if not task:
        return jsonify({}), 400
    return jsonify({'ID': task.id}), 202

