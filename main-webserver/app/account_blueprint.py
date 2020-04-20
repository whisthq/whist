from .imports import *
from .helperFuncs import *
from app import jwtManager

account_bp = Blueprint('account_bp', __name__)


@account_bp.route('/user/register', methods=['POST'])
def user_register():
    body = request.get_json()
    username, vm_name = body['username'], body['vm_name']
    vmExists = getVM(vm_name)
    vmInDatabase = singleValueQuery(vm_name)
    if vmExists and vmInDatabase:
        registerUserVM(username, vm_name)
        return jsonify({}), 200
    else:
        return jsonify({}), 403

@account_bp.route('/user/login', methods=['POST'])
def user_login():
    body = request.get_json()
    username = body['username']
    payload = fetchUserDisks(username)
    return jsonify({'payload': payload}), 200

@account_bp.route('/user/fetchvms', methods=['POST'])
@jwt_required
def user_fetchvms():
    body = request.get_json()
    username = body['username']
    try:
        return jsonify({'vms': fetchUserVMs(username)}), 200
    except Exception as e:
        return jsonify({}), 403

@account_bp.route('/user/fetchdisks', methods=['POST'])
def user_fetchdisks():
    body = request.get_json()
    disks = fetchUserDisks(body['username'])
    return jsonify({'disks': disks, 'status': 200}), 200

@account_bp.route('/user/reset', methods=['POST'])
@jwt_required
def user_reset():
    body = request.get_json()
    username, vm_name = body['username'], body['vm_name']
    resetVMCredentials(username, vm_name)
    return jsonify({'status': 200}), 200



# ACCOUNT endpoint

@account_bp.route('/account/login', methods=['POST'])
def account_login():
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
def account_register():
    body = request.get_json()
    username, password = body['username'], body['password']
    token = generateToken(username)
    status = registerUser(username, password, token)
    access_token, refresh_token = getAccessTokens(username)
    return jsonify({'status': status, 'token': token, 'access_token': access_token, 'refresh_token': refresh_token}), status

@account_bp.route('/account/checkVerified', methods=['POST'])
def account_check_verified():
    body = request.get_json()
    username = body['username']
    verified = checkUserVerified(username)
    return jsonify({'status': 200, 'verified': verified}), 200

@account_bp.route('/account/verifyUser', methods=['POST'])
@jwt_required
def account_verify_user():
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
def account_generate_ids():
    generateIDs()
    return jsonify({'status': 200}), 200

@account_bp.route('/account/fetchUsers', methods=['POST'])
@jwt_required
def account_fetch_users():
    users = fetchAllUsers()
    return jsonify({'status': 200, 'users': users}), 200
    
@account_bp.route('/account/delete', methods=['POST'])
@jwt_required
def account_delete():
    body = request.get_json()
    status = deleteUser(body['username'])
    return jsonify({'status': status}), status
    
@account_bp.route('/account/fetchCustomers', methods=['POST'])
@jwt_required
def account_fetch_customers():
    customers = fetchCustomers()
    return jsonify({'status': 200, 'customers': customers}), 200

@account_bp.route('/account/insertComputer', methods=['POST'])
@jwt_required
def account_insert_computer():
    body = request.get_json()
    username, location, nickname, computer_id = body[
        'username'], body['location'], body['nickname'], body['id']
    insertComputer(username, location, nickname, computer_id)
    return jsonify({'status': 200}), 200

@account_bp.route('/account/fetchComputers', methods=['POST'])
@jwt_required
def account_fetch_computers():
    body = request.get_json()
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

@account_bp.route('/account/checkComputer', methods=['POST'])
@jwt_required
def account_check_computer():
    body = request.get_json()
    computer_id, username = body['id'], body['username']
    computers = checkComputer(computer_id, username)
    return jsonify({'status': 200, 'computers': [computers]}), 200

@account_bp.route('/account/changeComputerName', methods=['POST'])
@jwt_required
def account_change_computer_name():
    body = request.get_json()
    username, nickname, computer_id = body['username'], body['nickname'], body['id']
    changeComputerName(username, computer_id, nickname)
    return jsonify({'status': 200}), 200

@account_bp.route('/account/regenerateCodes', methods=['POST'])
@jwt_required
def account_regenerate_codes():
    regenerateAllCodes()
    return jsonify({'status': 200}), 200

@account_bp.route('/account/fetchCode', methods=['POST'])
def account_fetch_code():
    body = request.get_json()
    code = fetchUserCode(body['username'])
    if code:
        return jsonify({'status': 200, 'code': code}), 200
    else:
        return jsonify({'status': 404, 'code': None}), 404


@account_bp.route('/account/feedback', methods=['POST'])
@jwt_required
def account_feedback():
    body = request.get_json()
    storeFeedback(body['username'], body['feedback'])
    return jsonify({'status': 200}), 200


@account_bp.route('/admin/<action>', methods=['POST'])
def admin(action):
    body = request.get_json()
    print(body)
    print(os.getenv('DASHBOARD_USERNAME'))
    print(os.getenv('DASHBOARD_PASSWORD'))
    if action == 'login':
        if body['username'] == os.getenv('DASHBOARD_USERNAME') and body['password'] == os.getenv('DASHBOARD_PASSWORD'):
            print("EQUAL")
            return jsonify({'status': 200}), 200
        return jsonify({'status': 422}), 422


# TOKEN endpoint (for access tokens)
@account_bp.route('/token/refresh', methods = ['POST'])
@jwt_refresh_token_required
def token_refresh():
	username = get_jwt_identity()
	access_token, refresh_token = getAccessTokens(username)
	return jsonify({'status': 200, 'username': username, 'access_token': access_token, 'refresh_token': refresh_token}), 200

@jwtManager.expired_token_loader
def my_expired_token_callback(expired_token):
    token_type = expired_token['type']
    return jsonify({
        'status': 401,
        'msg': 'The {} token has expired'.format(token_type)
    }), 401