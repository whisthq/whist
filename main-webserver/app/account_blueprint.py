from .imports import *
from .helperFuncs import *
from app import *

account_bp = Blueprint('account_bp', __name__)


@account_bp.route('/user/login', methods=['POST'])
@generateID
def user_login(**kwargs):
	sendInfo(kwargs['ID'], 'POST request sent to /user/login')
	body = request.get_json()
	username = body['username']
	payload = fetchUserDisks(username)
	return jsonify({'payload': payload}), 200

@account_bp.route('/user/fetchvms', methods=['POST'])
@jwt_required
@generateID
def user_fetchvms(**kwargs):
	sendInfo(kwargs['ID'], 'POST request sent to /user/fetchvms')
	body = request.get_json()
	username = body['username']
	try:
		return jsonify({'vms': fetchUserVMs(username)}), 200
	except Exception as e:
		return jsonify({}), 403

@account_bp.route('/user/fetchdisks', methods=['POST'])
@generateID
def user_fetchdisks(**kwargs):
	sendInfo(kwargs['ID'], 'POST request sent to /user/fetchdisks')
	body = request.get_json()
	disks = fetchUserDisks(body['username'])
	return jsonify({'disks': disks, 'status': 200}), 200

@account_bp.route('/user/reset', methods=['POST'])
@jwt_required
@generateID
def user_reset(**kwargs):
	sendInfo(kwargs['ID'], 'POST request sent to /user/reset')
	body = request.get_json()
	username, vm_name = body['username'], body['vm_name']
	resetVMCredentials(username, vm_name)
	return jsonify({'status': 200}), 200

@account_bp.route('/account/login', methods=['POST'])
@generateID
def account_login(**kwargs):
	sendInfo(kwargs['ID'], 'POST request sent to /account/login')
	body = request.get_json()
	username, password = body['username'], body['password']
	is_user = password != os.getenv('ADMIN_PASSWORD')
	verified = loginUser(username, password)
	vm_status = userVMStatus(username)
	token = fetchUserToken(username)
	access_token, refresh_token = getAccessTokens(username)
	return jsonify({'verified': verified,
	 'is_user': is_user, 
	 'vm_status': vm_status, 
	 'token': token, 
	 'access_token': access_token, 
	 'refresh_token': refresh_token}), 200

	
@account_bp.route('/account/register', methods=['POST'])
@generateID
def account_register(**kwargs):
	sendInfo(kwargs['ID'], 'POST request sent to /account/register')
	body = request.get_json()
	username, password = body['username'], body['password']
	token = generateToken(username)
	status = registerUser(username, password, token)
	access_token, refresh_token = getAccessTokens(username)
	return jsonify({'status': status, 'token': token, 'access_token': access_token, 'refresh_token': refresh_token}), status

@account_bp.route('/account/checkVerified', methods=['POST'])
@generateID
def account_check_verified(**kwargs):
	sendInfo(kwargs['ID'], 'POST request sent to /account/checkVerified')
	body = request.get_json()
	username = body['username']
	verified = checkUserVerified(username)
	return jsonify({'status': 200, 'verified': verified}), 200

@account_bp.route('/account/verifyUser', methods=['POST'])
@jwt_required
@generateID
def account_verify_user(**kwargs):
	sendInfo(kwargs['ID'], 'POST request sent to /account/verifyUser')
	body = request.get_json()
	username = body['username']
	correct_token = fetchUserToken(username)
	if body['token'] == correct_token:
		makeUserVerified(username, True)
		return jsonify({'status': 200, 'verified': True}), 200
	else:
		return jsonify({'status': 401, 'verified': False}), 401

@account_bp.route('/account/generateIDs', methods=['POST'])
@jwt_required
@generateID
def account_generate_ids(**kwargs):
	sendInfo(kwargs['ID'], 'POST request sent to /account/generateIDs')
	generateIDs()
	return jsonify({'status': 200}), 200

@account_bp.route('/account/fetchUsers', methods=['POST'])
@jwt_required
@generateID
def account_fetch_users(**kwargs):
	sendInfo(kwargs['ID'], 'POST request sent to /account/fetchUsers')
	users = fetchAllUsers()
	return jsonify({'status': 200, 'users': users}), 200
	
@account_bp.route('/account/delete', methods=['POST'])
@jwt_required
@generateID
def account_delete(**kwargs):
	sendInfo(kwargs['ID'], 'POST request sent to /account/delete')
	body = request.get_json()
	status = deleteUser(body['username'])
	return jsonify({'status': status}), status

@account_bp.route('/account/fetchCode', methods=['POST'])
@generateID
def account_fetch_code(**kwargs):
	sendInfo(kwargs['ID'], 'POST request sent to /account/fetchCode')
	body = request.get_json()
	code = fetchUserCode(body['username'])
	if code:
		return jsonify({'status': 200, 'code': code}), 200
	else:
		return jsonify({'status': 404, 'code': None}), 404


@account_bp.route('/account/feedback', methods=['POST'])
@jwt_required
@generateID
def account_feedback(**kwargs):
	sendInfo(kwargs['ID'], 'POST request sent to /account/feedback')
	body = request.get_json()
	storeFeedback(body['username'], body['feedback'])
	return jsonify({'status': 200}), 200


@account_bp.route('/admin/<action>', methods=['POST'])
@generateID
def admin(action, **kwargs):
	body = request.get_json()
	if action == 'login':
		sendInfo(kwargs['ID'], 'POST request sent to /admin/login')
		if body['username'] == os.getenv('DASHBOARD_USERNAME') and body['password'] == os.getenv('DASHBOARD_PASSWORD'):
			access_token, refresh_token = getAccessTokens(body['username'])
			return jsonify({'status': 200, 'access_token': access_token, 'refresh_token': refresh_token}), 200
		return jsonify({'status': 422}), 422


# TOKEN endpoint (for access tokens)
@account_bp.route('/token/refresh', methods = ['POST'])
@jwt_refresh_token_required
@generateID
def token_refresh(**kwargs):
	sendInfo(kwargs['ID'], 'POST request sent to /token/refresh')
	username = get_jwt_identity()
	access_token, refresh_token = getAccessTokens(username)
	return jsonify({'status': 200, 'username': username, 'access_token': access_token, 'refresh_token': refresh_token}), 200

@jwtManager.expired_token_loader
@generateID
def my_expired_token_callback(expired_token, **kwargs):
	token_type = expired_token['type']
	return jsonify({
		'status': 401,
		'msg': 'The {} token has expired'.format(token_type)
	}), 401