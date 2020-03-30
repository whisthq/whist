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
		username = body['username']
		username_len = len(username)

		vm_name = loginUserVM(username)

		try:
			if vm_name: 
				payload = fetchVMCredentials(vm_name)
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
		username, vm_name = body['username'], body['vm_name']
		resetVMCredentials(username, vm_name)
		return jsonify({'status': 200}), 200

@account_bp.route('/account/<action>', methods = ['POST'])
def account(action):
	body = request.get_json()
	if action == 'login':
		username, password = body['username'], body['password']
		is_user = password != os.getenv('ADMIN_PASSWORD')
		verified = loginUser(username, password)
		vm_status = userVMStatus(username)
		token = fetchUserToken(username)
		return jsonify({'verified': verified, 'is_user': is_user, 'vm_status': vm_status, 'token': token}), 200
	elif action == 'register':
		username, password = body['username'], body['password']
		token = generateToken(username)
		status = registerUser(username, password, token)
		return jsonify({'status': status, 'token': token}), status
	elif action == 'checkVerified':
		username = body['username']
		verified = checkUserVerified(username)
		return jsonify({'status': 200, 'verified': verified}), 200
	elif action == 'verifyUser':
		username = body['username']
		correct_token = fetchUserToken(username)
		if body['token'] == correct_token:
			makeUserVerified(username, True)
			return jsonify({'status': 200, 'verified': True}), 200
		else:
			return jsonify({'status': 401, 'verified': False}), 401
	elif action == 'generateIDs':
		generateIDs()
		return jsonify({'status': 200}), 200
	elif action == 'fetchUsers':
		users = fetchAllUsers()
		return jsonify({'status': 200, 'users': users}), 200
	elif action == 'delete':
		status = deleteUser(body['username'])
		return jsonify({'status': status}), status
	elif action == 'insertComputer':
		username, location, nickname, computer_id = body['username'], body['location'], body['nickname'], body['id']
		insertComputer(username, location, nickname, computer_id)
		return jsonify({'status': 200}), 200
	elif action == 'fetchComputers':
		username = body['username']
		computers = fetchComputers(username)
		i = 1
		proposed_nickname = 'Computer No. ' + str(i)
		nickname_exists = True
		number_computers = len(computers)
		computers_checked = 0

		while nickname_exists:
			computers_checked = 0
			for computer in computers:
				computers_checked += 1
				if computer['nickname'] == proposed_nickname:
					i += 1
					proposed_nickname = 'Computer No. ' + str(i)
					computers_checked = 0
			if computers_checked == number_computers:
				nickname_exists = False

		return jsonify({'status': 200, 'computers': computers}), 200
	elif action == 'checkComputer':
		computer_id, username = body['id'], body['username']
		computers = checkComputer(computer_id, username)
		return jsonify({'status': 200, 'computers': [computers]}), 200
	elif action == 'changeComputerName':
		username, nickname, computer_id = body['username'], body['nickname'], body['id']
		changeComputerName(username, computer_id, nickname)
		return jsonify({'status': 200}), 200
	elif action == 'regenerateCodes':
		regenerateAllCodes()
		return jsonify({'status': 200}), 200
	elif action == 'fetchCode':
		code = fetchUserCode(body['username'])
		if code:
			return jsonify({'status': 200, 'code': code}), 200
		else:
			return jsonify({'status': 404, 'code': None}), 404
	return jsonify({'status': 400}), 400


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