from app.tasks import *
from app import *

disk_bp = Blueprint('disk_bp', __name__)


@disk_bp.route('/disk/<action>', methods=['POST'])
@jwt_required
@generateID
def disk(action, **kwargs):
    if action == 'createEmpty':
        sendInfo(kwargs['ID'], 'POST request sent to /disk/createEmpty')

        body = request.get_json()
        disk_size = body['disk_size']
        username = body['username']
        location = body['location']
        task = createEmptyDisk.apply_async([disk_size, username, location])
        if not task:
            return jsonify({}), 400
        return jsonify({'ID': task.id}), 202
    elif action == 'createFromImage':
        sendInfo(kwargs['ID'], 'POST request sent to /disk/createFromImage')

        body = request.get_json()

        task = createDiskFromImage.apply_async(
            [body['username'], body['location'], body['vm_size']])
        if not task:
            return jsonify({}), 400
        return jsonify({'ID': task.id}), 202
    elif action == 'attach':
        sendInfo(kwargs['ID'], 'POST request sent to /disk/attach')

        body = request.get_json()
        task = swapDisk.apply_async([body['disk_name'], kwargs['ID']])
        return jsonify({'ID': task.id}), 202
    elif action == 'detach':
        sendInfo(kwargs['ID'], 'POST request sent to /disk/detach')

        vm_name = request.get_json()['vm_name']
        disk_name = request.get_json()['disk_name']
        task = detachDisk.apply_async([vm_name, disk_name])
        if not task:
            return jsonify({}), 400
        return jsonify({'ID': task.id}), 202
    elif action == 'resync':
        sendInfo(kwargs['ID'], 'POST request sent to /disk/resync')

        task = syncDisks.apply_async([])
        return jsonify({'ID': task.id}), 202
    elif action == 'update':
        sendInfo(kwargs['ID'], 'POST request sent to /disk/update')

        body = request.get_json()
        assignUserToDisk(body['disk_name'], body['username'])
        return jsonify({'status': 200}), 200
    elif action == 'delete':
        sendInfo(kwargs['ID'], 'POST request sent to /disk/delete')

        body = request.get_json()
        username = body['username']
        disks = fetchUserDisks(username, True)
        task_id = None

        if disks:
            for disk in disks:
                task = deleteDisk.apply_async([disk['disk_name']])
                task_id = task.id
        return jsonify({'ID': task_id}), 202
