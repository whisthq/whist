from .imports import *
from .tasks import *

bp = Blueprint("all", __name__)

@bp.route("/vm/<action>")
def makefile(action):
	if action == 'create':
	    task = createVM.apply_async()
	    print(task.id)
	    return jsonify({}), 202, {'ID': task.id}
	return jsonify({}), 400

@bp.route('/status/<task_id>')
def taskstatus(task_id):
    result = celery.AsyncResult(task_id)
    if result.status == 'SUCCESS':
        response = {
            'state': result.status,
            'output': result.result
        }
    elif result.status == 'FAILURE':
        # something went wrong in the background job
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