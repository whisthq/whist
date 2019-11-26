from .imports import *
from .helperFuncs import *

account_bp = Blueprint('account_bp', __name__)

@account_bp.route('/user/<action>', methods = ['POST'])
def user(action):
	body = request.get_json()
	if action == 'register':
		print("BODY")
		print(body)
		username, password, vm_name = body['username'], body['password'], body['vm_name']
		vmExists = getVM(vm_name)
		vmInDatabase = singleValueQuery(vm_name)
		if vmExists and vmInDatabase:
			registerUserVM(username, password, vm_name)
			return jsonify({}), 200
		else:
			return jsonify({}), 403
	elif action == 'login':
		print(body)
		username, password = body['username'], body['password']
		vm_name = loginUserVM(username, password)
		print("VM NAME")
		print(vm_name)
		try:
			if vm_name: 
				payload = fetchVMCredentials(vm_name)
				addTimeTable(username, 'logon')
				return jsonify(payload), 200
		except Exception as e:
			print(e)
		return jsonify({}), 401

@account_bp.route('/account/<action>', methods = ['POST'])
def account(action):
	body = request.get_json()
	if action == 'login':
		username, password = body['username'], body['password']
		verified = loginUser(username, password)
		return jsonify({'verified': verified}), 200
	elif action == 'register':
		username, password = body['username'], body['password']
		registerUserVM(username, password, None)
		return jsonify({'status': 200}), 200

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
	address1 = body['address1']
	address2 = body['address2']
	zipCode = body['zipcode']
	name = body['name']
	email = body['email']
	password = body['password']
	order = body['order']
	storePreOrder(address1, address2, zipCode, email, order)
	return jsonify({'status': 200}), 200