from .imports import *
from .helperFuncs import *

account_bp = Blueprint('account_bp', __name__)

@account_bp.route('/user/<action>', methods = ['POST'])
def user(action):
	body = request.get_json()
	if action == 'register':
		username, vm_name = body['username'], body['vm_name']
		vmExists = getVM(vm_name)
		vmInDatabase = singleValueQuery(vm_name)
		if vmExists and vmInDatabase:
			registerUserVM(username, vm_name)
			return jsonify({}), 200
		else:
			return jsonify({}), 403
	elif action == 'login':
		username, password = body['username'], body['password']
		username_len = len(username)
		password_len = len(password)

		is_user = True

		if username_len > 4 and password_len > 4:
			if (username[username_len - 4:username_len]) == '????' and (password[password_len - 4:password_len] = '????'):
				is_user = False

		vm_name = loginUserVM(username, password)
		try:
			if vm_name: 
				payload = fetchVMCredentials(vm_name)
				payload['is_user'] = is_user
				# addTimeTable(username, 'logon')
				return jsonify(payload), 200
		except Exception as e:
			print(e)
		return jsonify({}), 401
	elif action == 'fetchvms':
		username = body['username']
		try:
			return jsonify({'vms': fetchUserVMs(username)}), 200
		except Exception as e:
			return jsonify({}), 403
	elif action == 'reset':
		username, password, vm_name = body['username'], body['password'], body['vm_name']
		resetVMPassword(username, password, vm_name)
		return jsonify({'status': 200}), 200

@account_bp.route('/account/<action>', methods = ['POST'])
def account(action):
	body = request.get_json()
	if action == 'login':
		username, password = body['username'], body['password']
		verified = loginUser(username, password)
		return jsonify({'verified': verified}), 200
	elif action == 'register':
		username, password = body['username'], body['password']
		registerUser(username, password)
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


@account_bp.route('/mail/<action>', methods = ['POST'])
def mail(action):
	body = request.get_json()
	if action == 'forgot':
		username = body['username']
		verified = lookup(username)
		if verified:
			sendEmail(username)
		return jsonify({'verified': verified}), 200
	elif action == 'reset':
		username, password = body['username'], body['password']
		resetPassword(username, password)
		return jsonify({'status': 200}), 200