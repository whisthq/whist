from .imports import *
from .tasks import *

vm_bp = Blueprint('all', __name__)

@vm_bp.route('/status/<task_id>')
def status(task_id):
    result = celery.AsyncResult(task_id)
    if result.status == 'SUCCESS':
        response = {
            'state': result.status,
            'output': result.result
        }
    elif result.status == 'FAILURE':
        response = {
            'state': result.status,
            'output': str(result.info) 
        }
    else:
        response = {
            'state': result.status,
            'output': None
        }
    return jsonify(response)

@vm_bp.route('/vm/<action>', methods = ['POST'])
def vm(action):
    if action == 'create':
        vm_size = request.get_json()['vm_size']
        task = createVM.apply_async([vm_size])
        if not task: 
            return jsonify({}), 400
        return jsonify({'ID': task.id}), 202
    return jsonify({}), 400

