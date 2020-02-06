from .imports import *
from .tasks import *

vm_bp = Blueprint('vm_bp', __name__)

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

@vm_bp.route('/vm/<action>', methods = ['POST'])
def vm(action):
    if action == 'create':
        vm_size = request.get_json()['vm_size']
        task = createVM.apply_async([vm_size])
        if not task: 
            return jsonify({}), 400
        return jsonify({'ID': task.id}), 202
    elif action == 'fetchip':
        vm_name = request.get_json()['vm_name']
        try:
            vm = getVM(vm_name)
            print("test")
            ip = getIP(vm)
            return({'public_ip': ip}), 200
        except:
            return({'public_ip': None}), 404
    return jsonify({}), 400

@vm_bp.route('/tracker/<action>', methods = ['POST'])
def tracker(action):
    body = request.get_json()
    username = body['username']
    try:
        time = body['time']
    except: 
        time = None
    if action == 'logon':
        addTimeTable(username, 'logon', time)
    elif action == 'logoff':
        addTimeTable(username, 'logoff', time)
    elif action == 'clear':
        deleteTimeTable()
    return jsonify({}), 200

@vm_bp.route('/info/<action>', methods = ['GET', 'POST'])
def info(action):
    body = request.get_json()
    if action == 'list_all' and request.method == 'GET':
        task = fetchAll.apply_async([False])
        if not task:
            return jsonify({}), 400
        return jsonify({'ID': task.id}), 202
    if action == 'update_db' and request.method == 'POST':
        task = fetchAll.apply_async([True])
        if not task:
            return jsonify({}), 400
        return jsonify({'ID': task.id}), 202

    return jsonify({}), 400

