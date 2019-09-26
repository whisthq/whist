from .imports import *
from .tasks import *

vm_bp = Blueprint("all", __name__)

@vm_bp.route("/vm/<action>")
def vm(action):
	if action == 'create':
	    task = createVM.apply_async()
	    return jsonify({}), 202, {'ID': task.id}
	return jsonify({}), 400

@vm_bp.route('/status/<task_id>')
def taskstatus(task_id):
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