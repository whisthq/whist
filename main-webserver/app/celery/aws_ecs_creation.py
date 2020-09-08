from app import ECSClient, celery_instance, fractalLog, fractalSQLCommit, logging
from app.models.hardware import UserContainer
from app.serializers.hardware import UserContainerSchema

user_container_schema = UserContainerSchema()


def preprocess_task_info(taskinfo):
    # TODO:  actually write this
    return []


@celery_instance.task(bind=True)
def create_new_container(self, username, taskinfo):
    """

    Args:
        self: the celery instance running the task
        username (str): the username of the person creating the container
        taskinfo: info about the task to be run
        # TODO: fill in with types when known

    Returns:

    """
    fractalLog(
        function="create_new_container",
        label="None",
        logs="Creating new container on ECS with info {}".format(taskinfo),
    )

    ecs_client = ECSClient(launch_type='EC2')
    processed_task_info_cmd, processed_task_info_entrypoint, processed_image = preprocess_task_info(
        taskinfo
    )
    ecs_client.set_and_register_task(
        processed_task_info_cmd,
        processed_task_info_entrypoint,
        imagename=processed_image,
        family="fractal_container",
    )
    networkConfiguration = {
        "awsvpcConfiguration": {
            "subnets": ["subnet-0dc1b0c43c4d47945",],
            "securityGroups": ["sg-036ebf091f469a23e",],
        }
    }
    ecs_client.run_task(networkConfiguration=networkConfiguration)
    self.update_state(
        state="PENDING",
        meta={"msg": "Creating new container on ECS with info {}".format(taskinfo)},
    )
    ecs_client.spin_til_running(time_delay=2)
    curr_ip = ecs_client.task_ips.get(0, -1)
    # TODO:  Get this right
    curr_port = 80
    if curr_ip == -1:
        fractalLog(
            function="create_new_container",
            label=str(username),
            logs="Error generating task with running IP",
            level=logging.ERROR,
        )
        self.update_state(
            state="FAILURE", meta={"msg": "Error generating task with running IP"},
        )

        return
    container = UserContainer(
        container_id=ecs_client.tasks[0],
        user_id=username,
        cluster=ecs_client.cluster,
        ip=curr_ip,
        port=curr_port,
        state="CREATING",
        location="us-east-1",
        os="Linux",
        lock=False,
    )
    container_sql = fractalSQLCommit(db, lambda db, x: db.session.add(x), container)
    if container_sql:
        container = UserContainer.query.get(ecs_client.tasks[0])
        container = user_container_schema.dump(container)
        return container
    else:
        fractalLog(
            function="create_new_container",
            label=str(ecs_client.tasks[0]),
            logs="SQL insertion unsuccessful",)
        self.update_state(
            state="FAILURE",
            meta={
            "msg": "Error inserting VM {cli} and disk into SQL".format(cli=ecs_client.tasks[0])
            },
        )
        return None

@celery_instance.task(bind=True)
def create_new_cluster(self, instance_type, ami, min_size=1, max_size=10, region_name="us-east-2", availability_zones=None):
    fractalLog(
        function="create_new_cluster",
        label="None",
        logs=f"Creating new cluster on ECS with instance_type {instance_type} and ami {ami} in region {region_name}"
    )
    ecs_client = ECSClient(launch_type='EC2', region_name=region_name)

    ecs_client.run_task(networkConfiguration=networkConfiguration)
    self.update_state(
        state="PENDING",
        meta={
            "msg": f"Creating new cluster on ECS with instance_type {instance_type} and ami {ami} in region {region_name}"
        },
    )

    try:
        launch_config_name = ecs_client.create_launch_configuration(instance_type=instance_type, ami=ami, launch_config_name=None)
        auto_scaling_group_name = ecs_client.create_auto_scaling_group(launch_config_name=launch_config_name, min_size=min_size, max_size=max_size, availability_zones=availability_zones)
        capacity_provider_name = ecs_client.create_capacity_provider(auto_scaling_group_name=auto_scaling_group_name)
        return ecs_client.create_cluster(capacity_providers=[capacity_provider_name])
    except Exception as e:
        fractalLog(
            function="create_new_cluster",
            logs=f"Encountered error: {e}",
            level=logging.ERROR,
        )
        self.update_state(
            state="FAILURE",
            meta={
                "msg": f"Encountered error: {e}"
            },
        )
