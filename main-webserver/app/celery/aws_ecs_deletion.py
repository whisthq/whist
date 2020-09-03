from app import *
from app.helpers.utils.aws.aws_resource_locks import *
from app.helpers.utils.aws.base_ecs_client import *
from app.models.hardware import *
from app.serializers.hardware import *


@celery_instance.task(bind=True)
def deleteContainer(self, container_name):

    if spinLock(container_name) < 0:
        return {"status": REQUEST_TIMEOUT}
    fractalLog(
        function="deleteContainer",
        label=str(container_name),
        logs="Beginning to delete Container {container_name}. Goodbye, {container_name}!".format(
            container_name=container_name
        ),
    )

    lockContainerAndUpdate(
        container_name=container_name, state="DELETING", lock=True, temporary_lock=10
    )

    container = UserContainer.query.get(container_name)
    container_cluster = container.cluster
    ecs_client = ECSClient(base_cluster=container_cluster, grab_logs=False)
    ecs_client.add_task(container_name)
    try:
        ecs_client.stop_task(reason='API triggered task stoppage', offset=0)
        self.update_state(
            state="PENDING",
            meta={
                "msg": "Container {container_name} begun stoppage".format(
                    container_name=container_name,
                )
            },
        )
        ecs_client.spin_til_done(offset=0)
        fractalSQLCommit(db, lambda db, x: db.session.delete(x), container)
    except Exception as e:
        fractalLog(
            function="deleteContainer",
            label=str(container_name),
            logs="ran into deletion error {}".format(e),
        )

        self.update_state(
            state="FAILURE",
            meta={
                "msg": "Error stopping Container {container_name}".format(
                    container_name=container_name
                )
            },
        )
        return {'status': INTERNAL_SERVER_ERROR}
    return {"status": SUCCESS}
