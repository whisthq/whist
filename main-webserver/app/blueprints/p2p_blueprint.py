from app import *
from app.helpers.studios import *
from app.helpers.customers import *

p2p_bp = Blueprint("p2p_bp", __name__)


@p2p_bp.route("/account/fetchCustomers", methods=["POST"])
@jwt_required
@generateID
@logRequestInfo
def account_fetch_customers(**kwargs):
    sendInfo(kwargs["ID"], "POST request sent to /account/fetchCustomers")
    customers = fetchCustomers()
    return jsonify({"status": 200, "customers": customers}), 200


@p2p_bp.route("/account/insertComputer", methods=["POST"])
@jwt_required
@generateID
@logRequestInfo
def account_insert_computer(**kwargs):
    sendInfo(kwargs["ID"], "POST request sent to /account/insertComputer")
    body = request.get_json()
    username, location, nickname, computer_id = (
        body["username"],
        body["location"],
        body["nickname"],
        body["id"],
    )
    insertComputer(username, location, nickname, computer_id)
    return jsonify({"status": 200}), 200


@p2p_bp.route("/account/fetchComputers", methods=["POST"])
@jwt_required
@generateID
@logRequestInfo
def account_fetch_computers(**kwargs):
    sendInfo(kwargs["ID"], "POST request sent to /account/fetchComputers")
    body = request.get_json()
    username = body["username"]
    computers = fetchComputers(username)
    i = 1
    proposed_nickname = "Computer No. " + str(i)
    nickname_exists = True
    number_computers = len(computers)
    computers_checked = 0

    while nickname_exists:
        computers_checked = 0
        for computer in computers:
            computers_checked += 1
            if computer["nickname"] == proposed_nickname:
                i += 1
                proposed_nickname = "Computer No. " + str(i)
                computers_checked = 0
        if computers_checked == number_computers:
            nickname_exists = False

    return jsonify({"status": 200, "computers": computers}), 200


@p2p_bp.route("/account/checkComputer", methods=["POST"])
@jwt_required
@generateID
@logRequestInfo
def account_check_computer(**kwargs):
    sendInfo(kwargs["ID"], "POST request sent to /account/checkComputer")
    body = request.get_json()
    computer_id, username = body["id"], body["username"]
    computers = checkComputer(computer_id, username)
    return jsonify({"status": 200, "computers": [computers]}), 200


@p2p_bp.route("/account/changeComputerName", methods=["POST"])
@jwt_required
@generateID
@logRequestInfo
def account_change_computer_name(**kwargs):
    sendInfo(kwargs["ID"], "POST request sent to /account/changeComputerName")
    body = request.get_json()
    username, nickname, computer_id = body["username"], body["nickname"], body["id"]
    changeComputerName(username, computer_id, nickname)
    return jsonify({"status": 200}), 200
