from .imports import *
from .helperFuncs import *

vm_bp = Blueprint('all', __name__)

@vm_bp.route('/user/<action>', methods = ['POST'])
def user(action):
	body = request.get_json()
	if action == 'register':
		username, password, vm_name = body['username'], body['password'], body['vm_name']
		registerUser(username, vm_name)
		return jsonify({}), 200
	elif action == 'login':
		username = body['username']
		vm_name = loginUser(username)
		if vm_name: 
			payload = fetchVMCredentials(vm_name)
			return jsonify(payload), 200
		return jsonify({}), 401