#TODO:  create blueprints for:  getting container info, starting a new container given a task and a user, stopping a container
#this file is for Owen's work
from app import *
from app.celery.aws_ecs_creation import *

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
