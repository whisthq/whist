from .imports import *
from .helperFuncs import *

account_bp = Blueprint('account_bp', __name__)

@account_bp.route('/user/<action>', methods = ['POST'])
def user(action):
	body = request.get_json()
	if action == 'register':
		username, password, vm_name = body['username'], body['password'], body['vm_name']
		vmExists = getVM(vm_name)
		vmInDatabase = singleValueQuery(vm_name)
		if vmExists and vmInDatabase:
			registerUser(username, password, vm_name)
			return jsonify({}), 200
		else:
			return jsonify({}), 403
	elif action == 'login':
		username, password = body['username'], body['password']
		vm_name = loginUser(username, password)
		if vm_name: 
			payload = fetchVMCredentials(vm_name)
			return jsonify(payload), 200
		return jsonify({}), 401

@account_bp.route('/form/<action>', methods = ['POST'])
def form(action):
	body = request.get_json()
	if action == 'store':
		name, email, cubeType = body['name'], body['email'], body['cubeType']
		storeForm(name, email, cubeType)
		return jsonify({'status': 200}), 200

@account_bp.route('/order', methods = ['POST'])
def order():
	body = request.get_json()
	print(body)
	address1 = body['address1']
	address2 = body['address2']
	zipCode = body['zipcode']
	name = body['name']
	email = body['email']
	password = body['password']
	order = body['order']
	storePreOrder(address1, address2, zipCode, email, order)
	return jsonify({'status': 200}), 200