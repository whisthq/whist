# TODO:  create blueprints for:  getting container info, starting a new container given a task and a user, stopping a container
# this file is for Owen's work

from app import *
from app.helpers.utils.general.auth import *

from app.helpers.utils.aws.aws_general import *

from app.celery.aws_ecs_creation import *
from app.celery.aws_ecs_deletion import *

from app.models.hardware import *

aws_container_bp = Blueprint("aws_container_bp", __name__)


@aws_container_bp.route("/aws_container/<action>", methods=["POST"])
@fractalPreProcess
def test_endpoint(action, **kwargs):
    if action == "create_cluster":
        instance_type, ami = (
            kwargs["body"]["instance_type"],
            kwargs["body"]["ami"],
        )
        task = create_new_cluster.apply_async([instance_type, ami])

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED

    if action == "launch_task":
        username, cluster_name = (
            kwargs["body"]["username"],
            kwargs["body"]["cluster_name"],
        )
        task = create_new_container.apply_async([username, cluster_name])

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED


@aws_container_bp.route("/container/<action>", methods=["POST"])
@fractalPreProcess
@jwt_required
@fractalAuth
def aws_container_post(action, **kwargs):
    if action == "create":
        # TODO: Read HTTP request body, call the container creation function in celery.aws_ecs_creation and return a
        # task ID. See the "clone" endpoint in blueprints.azure.azure_disk_blueprint for inspiration.
        pass

    elif action == "delete":
        # TODO: Read HTTP request body, call the container deletion function in celery.aws_ecs_deletion and return a
        # task ID. See the "delete" endpoint in blueprints.azure.azure_disk_blueprint for inspiration.
        pass

    elif action == "restart":
        # TODO: Same as above, but inspire yourself from the vm/restart endpoint.
        pass

    elif action == "ping":
        # TODO: Read the ip address of the sender, and pass it into a helper function that updates the timestamp
        # of the container in SQL, marks it as either RUNNING_AVAILABLE/RUNNING_UNAVAILABLE, etc. Basically,
        # migrate the vm/ping endpoint blueprints.azure.azure_vm_blueprint for containers.
        pass

    elif action == "stun":
        # TODO: Toggle whether a container uses stun. Inspire yourself from the disk/stun endpoint. You'll need to
        # create a using_stun column in the user_containers table, which you can do from TablePlus. When you create
        # a new column, make sure to modify the appropriate class in the /models folder.
        pass

    elif action == "protocol_info":
        # TODO: Same as /stun, migrate over disk/protocol_info
        pass
