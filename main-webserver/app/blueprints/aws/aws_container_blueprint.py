#TODO:  create blueprints for:  getting container info, starting a new container given a task and a user, stopping a container
#this file is for Owen's work
from app import *
from app.celery.aws_ecs_creation import *
from app.celery.aws_ecs_deletion import *

aws_container_bp = Blueprint("aws_container_bp", __name__)

@aws_container_bp.route("/aws_container/<action>", methods=["POST"])
@fractalPreProcess
def test_endpoint(action, **kwargs):
    if action == "create_cluster":
        instance_type, ami, region_name = (
            kwargs["body"]["instance_type"],
            kwargs["body"]["ami"],
            kwargs["body"]["region_name"],
        )
        task = create_new_cluster.apply_async([instance_type, ami, region_name])

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED

    if action == "create_container":
        username, cluster_name, region_name, task_definition_arn, use_launch_type, network_configuration = (
            kwargs["body"]["username"],
            kwargs["body"]["cluster_name"],
            kwargs["body"]["region_name"],
            kwargs["body"]["task_definition_arn"],
            kwargs["body"]["use_launch_type"],
            kwargs["body"]["network_configuration"],
        )
        task = create_new_container.apply_async([username, cluster_name, region_name, task_definition_arn, use_launch_type, network_configuration])

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED

    if action == "delete_container":
        user_id, container_name = (
            kwargs["body"]["user_id"],
            kwargs["body"]["container_name"],
        )
        task = deleteContainer.apply_async([user_id, container_name])

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED