from app.tasks import *
from app import *
from app.helpers.customers import *
from app.helpers.vms import *
from app.helpers.logins import *
from app.helpers.disks import *
from app.helpers.s3 import *
from app.helpers.users import *

vm_bp = Blueprint("vm_bp", __name__)

# To access jwt_required endpoints, must include
# Authorization: Bearer <Access Token JWT>
# in header of request

# To access jwt_refresh_token_required endpoints, must include
# Authorization: Bearer <Refresh Token JWT>
# in header of request


@vm_bp.route("/status/<task_id>")
@generateID
@logRequestInfo
def status(task_id, **kwargs):
    result = celery.AsyncResult(task_id)
    if result.status == "SUCCESS":
        response = {
            "state": result.status,
            "output": result.result,
        }
        return make_response(jsonify(response), 200)
    elif result.status == "FAILURE":
        response = {"state": result.status, "output": result.info}
        return make_response(jsonify(response), 200)
    elif result.status == "PENDING":
        response = {"state": result.status, "output": result.info}
        return make_response(jsonify(response), 200)
    else:
        response = {"state": result.status, "output": None}
        return make_response(jsonify(response), 200)


@vm_bp.route("/vm/<action>", methods=["POST", "GET"])
@generateID
@logRequestInfo
def vm(action, **kwargs):
    if action == "create" and request.method == "POST":
        body = request.get_json()
        vm_size = body["vm_size"]
        location = body["location"]
        operating_system = "Windows"
        admin_password = None
        if "admin_password" in body.keys():
            admin_password = body["admin_password"]

        if "operating_system" in body.keys():
            operating_system = body["operating_system"]

        task = createVM.apply_async([vm_size, location, operating_system, admin_password, kwargs["ID"]])
        if not task:
            return jsonify({}), 400
        return jsonify({"ID": task.id}), 202
    elif action == "fetchip" and request.method == "POST":
        vm_name = request.get_json()["vm_name"]
        try:
            vm = getVM(vm_name)
            ip = getIP(vm)
            sendInfo(kwargs["ID"], "Ip found for VM {}".format(vm_name))
            return ({"public_ip": ip}), 200
        except:
            sendError(kwargs["ID"], "Ip not found for VM {}".format(vm_name))
            return ({"public_ip": None}), 404
    elif action == "setDev" and request.method == "POST":
        vm_name = request.get_json()["vm_name"]
        dev = request.get_json()["dev"]
        setDev(vm_name, dev)
        sendInfo(kwargs["ID"], "Set dev state for vm {} to {}".format(vm_name, dev))
        return jsonify({"status": 200}), 200
    elif action == "delete" and request.method == "POST":
        body = request.get_json()

        vm_name, delete_disk = body["vm_name"], body["delete_disk"]
        task = deleteVMResources.apply_async([vm_name, delete_disk])
        return jsonify({"ID": task.id}), 202
    elif action == "restart" and request.method == "POST":
        username = request.get_json()["username"]
        vm = fetchUserVMs(username)
        if vm:
            vm_name = vm[0]["vm_name"]
            task = restartVM.apply_async([vm_name, kwargs["ID"]])
            return jsonify({"ID": task.id}), 202
        sendInfo(kwargs["ID"], "No vms found for user {}".format(username))
        return jsonify({"ID": None}), 404
    elif action == "start":
        vm_name = request.get_json()["vm_name"]
        task = startVM.apply_async([vm_name, kwargs["ID"]])
        return jsonify({"ID": task.id}), 202
    elif action == "stopvm":
        vm_name = request.get_json()["vm_name"]
        task = stopVM.apply_async([vm_name, kwargs["ID"]])
        return jsonify({"ID": task.id}), 202
    elif action == "deallocate":
        vm_name = request.get_json()["vm_name"]
        task = deallocateVM.apply_async([vm_name, kwargs["ID"]])
        return jsonify({"ID": task.id}), 202
    elif action == "updateState" and request.method == "POST":
        task = updateVMStates.apply_async([kwargs["ID"]])
        return jsonify({"ID": task.id}), 202
    elif action == "diskSwap" and request.method == "POST":
        body = request.get_json()
        task = swapSpecificDisk.apply_async(
            [body["disk_name"], body["vm_name"], kwargs["ID"]]
        )
        return jsonify({"ID": task.id}), 202
    elif action == "updateTable" and request.method == "POST":
        task = updateVMTable.apply_async([kwargs["ID"]])
        return jsonify({"ID": task.id}), 202
    elif action == "fetchall" and request.method == "POST":
        body = request.get_json()
        vms = fetchUserVMs(None, kwargs["ID"])
        return jsonify({"payload": vms, "status": 200}), 200
    elif action == "winlogonStatus" and request.method == "POST":
        body = request.get_json()
        ready = body["ready"]
        vm_ip = ""
        if request.headers.getlist("X-Forwarded-For"):
            vm_ip = request.headers.getlist("X-Forwarded-For")[0]
        else:
            vm_ip = request.environ["HTTP_X_FORWARDED_FOR"]

        sendInfo(
            kwargs["ID"],
            "Received winlogon update from ip {} with ready state {}".format(
                vm_ip, ready
            ),
        )

        vm_info = fetchVMByIP(vm_ip)

        if vm_info:
            vm_name = vm_info["vm_name"]
            vmReadyToConnect(vm_name, ready)
        else:
            sendError(kwargs["ID"], "No VM found for IP {}".format(str(vm_ip)))

        return jsonify({"status": 200}), 200
    elif action == "connectionStatus" and request.method == "POST":
        body = request.get_json()
        available = body["available"]
        vm_ip = ""
        if request.headers.getlist("X-Forwarded-For"):
            vm_ip = request.headers.getlist("X-Forwarded-For")[0]
        else:
            vm_ip = request.environ["HTTP_X_FORWARDED_FOR"]

        vm_info = fetchVMByIP(vm_ip)
        if vm_info:
            vm_name = vm_info["vm_name"] if vm_info["vm_name"] else ""

            if vm_info["os"] == "Linux":
                vmReadyToConnect(vm_info["vm_name"], True)

            version = None
            if "version" in body:
                version = body["version"]
                updateProtocolVersion(vm_name, version)

            vm_state = vm_info["state"] if vm_info["state"] else ""
            intermediate_states = ["STOPPING", "DEALLOCATING", "ATTACHING"]
            username = vm_info["username"]

            disks = fetchUserDisks(vm_info["username"], ID = kwargs["ID"])
            is_user = True
            if disks:
                disk = disks[0]
                is_user = not disk["branch"] == "dev" and not vm_info["dev"]

            # Update the user's login status if it has changed
            if available and vm_state == "RUNNING_UNAVAILABLE":
                # THIS IS A LOGOFF EVENT, bc it is now available and the last recorded state was unavailable
                sendInfo(kwargs["ID"], username + " logged off")
                customer = fetchCustomer(username)
                if not customer:
                    sendWarning(
                        kwargs["ID"],
                        "{} logged on/off but is not a registered customer".format(
                            username
                        ),
                    )
                else:
                    stripe.api_key = os.getenv("STRIPE_SECRET")
                    subscription_id = customer["subscription"]

                    try:
                        payload = stripe.Subscription.retrieve(subscription_id)

                        if (
                            os.getenv("HOURLY_PLAN_ID")
                            == payload["items"]["data"][0]["plan"]["id"]
                        ):
                            sendInfo(
                                kwargs["ID"],
                                "{} is an hourly plan subscriber".format(username),
                            )
                            user_activity = getMostRecentActivity(username)
                            if user_activity["action"] == "logon":
                                now = dt.now()
                                logon = dt.strptime(
                                    user_activity["timestamp"], "%m-%d-%Y, %H:%M:%S"
                                )
                                if now - logon > timedelta(minutes=0):
                                    amount = round(
                                        79 * (now - logon).total_seconds() / 60 / 60
                                    )
                                    addPendingCharge(username, amount)
                            else:
                                sendError(
                                    kwargs["ID"],
                                    "{} logged off but no logon was recorded".format(
                                        username
                                    ),
                                )
                    except:
                        sendError(
                            kwargs["ID"],
                            "User {} logged off but there was an issue charging with stripe".format(
                                username
                            ),
                        )
                    addTimeTable(username, "logoff", is_user, kwargs["ID"])
            elif not available and vm_state == "RUNNING_AVAILABLE":
                # THIS IS A LOGON EVENT, bc it is now unavailable and the last recorded state was available
                sendInfo(kwargs["ID"], username + " logged on")
                addTimeTable(username, "logon", is_user, kwargs["ID"])
                vms = fetchUserVMs(username)
                if vms:
                    createTemporaryLock(vms[0]["vm_name"], 0, kwargs["ID"])

            # Updates the vm state in the sql database
            if vm_state in intermediate_states:
                sendWarning(
                    kwargs["ID"],
                    "Trying to change connection status to {}, but VM {} is in intermediate state {}. Not changing state.".format(
                        "RUNNING_AVAILABLE" if available else "RUNNING_UNAVAILABLE",
                        vm_name,
                        vm_state,
                    ),
                )
            if available and not vm_state in intermediate_states:
                lockVMAndUpdate(
                    vm_name=vm_name,
                    state="RUNNING_AVAILABLE",
                    lock=False,
                    temporary_lock=None,
                    change_last_updated=False,
                    verbose=False,
                    ID=kwargs["ID"],
                )
            elif not available and not vm_state in intermediate_states:
                lockVMAndUpdate(
                    vm_name=vm_name,
                    state="RUNNING_UNAVAILABLE",
                    lock=True,
                    temporary_lock=0,
                    change_last_updated=False,
                    verbose=False,
                    ID=kwargs["ID"],
                )

        else:
            sendError(
                kwargs["ID"],
                "Trying to change connection status, but no VM found for IP {}".format(
                    str(vm_ip)
                ),
            )

        return jsonify({"status": 200}), 200
    elif action == "isDev" and request.method == "GET":
        try:
            vm_ip = ""

            if request.headers.getlist("X-Forwarded-For"):
                vm_ip = request.headers.getlist("X-Forwarded-For")[0]
            else:
                vm_ip = request.remote_addr

            vm_info = fetchVMByIP(vm_ip)

            if vm_info:
                is_dev = vm_info["dev"]
                disk_name = vm_info["disk_name"]
                disk_info = fetchUserDisks(vm_info["username"])

                branch = None
                if disk_info:
                    branch = disk_info[0]["branch"]

                using_stun = fetchDiskSetting(
                    disk_name,
                    "using_stun"
                )

                return jsonify({
                    "dev": is_dev, 
                    "branch": branch, 
                    "status": 200,
                    "using_stun": using_stun
                }), 200
            return jsonify({"dev": False, "status": 200}), 200
        except Exception as e:
            print(str(e))
    elif action == "setDev" and request.method == "POST":
        vm_name = request.get_json()["vm_name"]
        dev = request.get_json()["dev"]
        setDev(vm_name, dev)
        sendInfo(kwargs["ID"], "Set dev state for vm {} to {}".format(vm_name, dev))
        return jsonify({"status": 200}), 200


    return jsonify({}), 400


@vm_bp.route("/tracker/<action>", methods=["POST"])
@jwt_required
@generateID
@logRequestInfo
def tracker(action, **kwargs):
    body = request.get_json()
    time = None
    try:
        time = body["time"]
    except:
        pass
    if action == "fetch":
        activity = fetchLoginActivity(kwargs["ID"])
        return jsonify({"payload": activity}), 200
    elif action == "fetchMostRecent":
        activity = getMostRecentActivity(body["username"], kwargs["ID"])
        return jsonify({"payload": activity})
    return jsonify({}), 200


# INFO endpoint

@vm_bp.route("/info/<action>", methods=["GET", "POST"])
@jwt_required
@generateID
@logRequestInfo
def info(action, **kwargs):
    body = request.get_json()
    if action == "list_all" and request.method == "GET":
        task = fetchAll.apply_async([False])
        if not task:
            return jsonify({}), 400
        return jsonify({"ID": task.id}), 202
    if action == "list_all_disks" and request.method == "GET":
        disks = fetchUserDisks(None)
        return jsonify({"disks": disks}), 200
    if action == "update_db" and request.method == "POST":
        task = fetchAll.apply_async([True])
        if not task:
            return jsonify({}), 400
        return jsonify({"ID": task.id}), 202

    return jsonify({}), 400


@vm_bp.route("/logs", methods=["POST"])
@generateID
@logRequestInfo
def logs(**kwargs):
    body = json.loads(request.data)

    vm_ip = None
    if "vm_ip" in body:
        vm_ip = body["vm_ip"]
        sendInfo(kwargs["ID"], "Logs came from {}".format(body["vm_ip"]))
    else:
        if request.headers.getlist("X-Forwarded-For"):
            vm_ip = request.headers.getlist("X-Forwarded-For")[0]
        else:
            vm_ip = request.remote_addr

    sendInfo(
        kwargs["ID"],
        "Logs received from {} with connection ID {} and IP {}".format(
            body["sender"], str(body["connection_id"]), str(vm_ip)
        ),
    )

    version = None
    if "version" in body:
        version = body["version"]
        sendInfo(kwargs["ID"], "Logs came from version {}".format(body["version"]))

    task = storeLogs.apply_async(
        [
            body["sender"],
            body["connection_id"],
            body["logs"],
            vm_ip,
            version,
            kwargs["ID"],
        ]
    )
    return jsonify({"ID": task.id}), 202


@vm_bp.route("/logs/<action>", methods=["POST"])
@generateID
@logRequestInfo
def logs_actions(action, **kwargs):
    body = request.get_json()
    # fetch logs action
    if action == "fetch" and request.method == "POST":
        try:
            fetch_all = body["fetch_all"]
        except:
            fetch_all = False

        task = fetchLogs.apply_async([body["username"], fetch_all, kwargs["ID"]])
        return jsonify({"ID": task.id}), 202
    # delete logs action
    elif action == "delete" and request.method == "POST":
        try:
            connection_id = body["connection_id"]
        except:
            sendError(ID, "No connection id received")
            # should always be a connection_id
            return jsonify({}), 400

        task = deleteLogs.apply_async([connection_id, kwargs["ID"]])
        return jsonify({"ID": task.id}), 202
