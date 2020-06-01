from app.tasks import *
from app import *
from app.helpers.disks import *
from app.helpers.versions import *
from app.logger import *

disk_bp = Blueprint("disk_bp", __name__)


@disk_bp.route("/disk/<action>", methods=["POST"])
@jwt_required
@generateID
@logRequestInfo
def disk(action, **kwargs):
    if action == "createEmpty":
        body = request.get_json()

        disk_size = body["disk_size"]
        username = body["username"]

        disks = fetchUserDisks(username, main=True)
        if disks:
            location = disks[0]["location"]
            task = createEmptyDisk.apply_async(
                [disk_size, username, location, kwargs["ID"]]
            )
            if not task:
                return jsonify({}), 400
            return jsonify({"ID": task.id}), 202
        else:
            return jsonify({"ID": None}), 404
    elif action == "createFromImage":
        body = request.get_json()

        operating_system = "Windows"
        if "operating_system" in body.keys():
            operating_system = body["operating_system"]

        task = createDiskFromImage.apply_async(
            [
                body["username"],
                body["location"],
                body["vm_size"],
                operating_system,
                kwargs["ID"],
            ]
        )

        if not task:
            sendError(kwargs["ID"], "Error creating disk from image")
            return jsonify({}), 400
        return jsonify({"ID": task.id}), 202
    elif action == "attach":
        # Attaches a disk to an available vm in the region. If an available vm has a disk, swap the disks.
        body = request.get_json()
        task = swapDiskSync.apply_async([body["disk_name"], kwargs["ID"]])
        return jsonify({"ID": task.id}), 202
    elif action == "add":
        body = request.get_json()

        task = attachDisk.apply_async(
            [body["disk_name"], body["vm_name"], kwargs["ID"]]
        )
        return jsonify({"ID": task.id}), 202
    elif action == "detach":
        body = request.get_json()

        task = detachDisk.apply_async(
            [body["disk_name"], body["vm_name"], kwargs["ID"]]
        )
        if not task:
            return jsonify({}), 400
        return jsonify({"ID": task.id}), 202
    elif action == "resync":
        task = syncDisks.apply_async([kwargs["ID"]])
        return jsonify({"ID": task.id}), 202
    elif action == "update":
        body = request.get_json()
        assignUserToDisk(body["disk_name"], body["username"])
        return jsonify({"status": 200}), 200
    elif action == "acceptedUpdate":
        body = request.get_json()
        setUpdateAccepted(body["disk_name"], body["accepted"], kwargs["ID"])
        return jsonify({"status": 200}), 200
    elif action == "acceptedUpdateFetch":
        body = request.get_json()
        accepted = setUpdateAccepted(body["disk_name"], kwargs["ID"])
        if accepted is None:
            return jsonify({}), 400
        return jsonify({"accepted": accepted}), 200
    elif action == "delete":
        body = request.get_json()
        username = body["username"]
        sendInfo(
            kwargs["ID"], "Deleting disks associated with user {}".format(username)
        )
        disks = fetchUserDisks(username, True)
        task_id = None

        if disks:
            for disk in disks:
                task = deleteDisk.apply_async([disk["disk_name"]])
                task_id = task.id
        sendInfo(kwargs["ID"], "User disk deletion complete")
        return jsonify({"ID": task_id}), 202
    elif action == "deleteSpecific":
        body = request.get_json()
        disk_name = body["disk_name"]
        sendInfo(kwargs["ID"], "Deleting disk {}".format(disk_name))
        task_id = None

        task = deleteDisk.apply_async(disk_name)
        task_id = task.id
        sendInfo(kwargs["ID"], "Disk deletion complete")
        return jsonify({"ID": task_id}), 202
    elif action == "setVersion":
        body = request.get_json()
        setDiskVersion(body["disk_name"], body["branch"])
        return jsonify({"status": 200}), 200


@disk_bp.route("/version/<action>", methods=["POST"])
@generateID
@logRequestInfo
def version(action, **kwargs):
    if action == "set":
        body = request.get_json()
        setBranchVersion(body["branch"], body["version"], kwargs["ID"])
        return jsonify({"status": 200}), 200
    elif action == "get":
        body = request.get_json()
        branch = body["branch"]
        try:
            version = getBranchVersion(branch, kwargs["ID"])
            sendInfo(kwargs["ID"], "Version found for branch {}".format(branch))
            return ({"version": version}), 200
        except:
            sendError(
                kwargs["ID"], "Version found not found for branch {}".format(branch)
            )
            return ({"version": None}), 404
