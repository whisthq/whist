from app import ECSClient, celery_instance, fractalLog, fractalSQLCommit, logging
from app.models.hardware import UserContainer
from app.serializers.hardware import UserContainerSchema

user_container_schema = UserContainerSchema()


def preprocess_task_info(taskinfo):
    # TODO:  actually write this
    return []


@celery_instance.task(bind=True)
def create_new_container(self, username, taskinfo):
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
            logs="SQL insertion unsuccessful",
        )
        self.update_state(
            state="FAILURE",
            meta={
                "msg": "Error inserting VM {cli} and disk into SQL".format(cli=ecs_client.tasks[0])
            },
        )
        return None
