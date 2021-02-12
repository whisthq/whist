from app.helpers.utils.aws.base_ecs_client import ECSClient
from app.helpers.utils.general.sql_commands import fractal_sql_commit
from app.helpers.utils.general.logs import fractal_log
from app.models import db


def ensure_container_exists(container):
    """
    Function determining whether a container in the database actually
    exists as a task in ECS. If the container does not exist, delete
    it from the database.

    After a successful call to this function, the invariant is set that
    any non-Null return is a real container corresponding to an ECS
    task.

    This function should only be called after acquiring a lock on the
    container in question!

    :param container: the UserContainer entry whose task to find
    :return: `container` if the task exists, else `None`
    """

    if container is None:
        raise Exception("Invalid parameter: `container` is `None`")

    ecs_client = ECSClient(
        base_cluster=container.cluster, region_name=container.location, grab_logs=False
    )

    if not ecs_client.check_task_exists(container.container_name):
        # the task doesn't really exist! we should delete from the database
        fractal_sql_commit(db, lambda db, x: db.session.delete(x), container)
        fractal_log(
            function="ensure_container_exists",
            label=container.container_name,
            logs=f"Task was not in region {container.location} cluster {container.cluster}!"
            "Deleted erroneous entry from database.",
        )
        return None
    return container
