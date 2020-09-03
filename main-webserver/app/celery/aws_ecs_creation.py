from app import (
    ECSClient,
    celery_instance,
    fractalLog,
    fractalSQLCommit,
    fractalSQLUpdate,
    logging,
    db,
)
from app.models.hardware import UserContainer, ClusterInfo
from app.serializers.hardware import UserContainerSchema, ClusterInfoSchema
import time

user_container_schema = UserContainerSchema()
user_cluster_schema = ClusterInfoSchema()

base_task = {
    "executionRoleArn": "arn:aws:iam::747391415460:role/ecsTaskExecutionRole",
    "containerDefinitions": [
        {
            "portMappings": [
                {"hostPort": 32262, "protocol": "tcp", "containerPort": 32262},
                {"hostPort": 32263, "protocol": "udp", "containerPort": 32263},
                {"hostPort": 32273, "protocol": "tcp", "containerPort": 32273},
            ],
            "linuxParameters": {
                "capabilities": {
                    "add": [
                        "SETPCAP",
                        "MKNOD",
                        "AUDIT_WRITE",
                        "CHOWN",
                        "NET_RAW",
                        "DAC_OVERRIDE",
                        "FOWNER",
                        "FSETID",
                        "KILL",
                        "SETGID",
                        "SETUID",
                        "NET_BIND_SERVICE",
                        "SYS_CHROOT",
                        "SETFCAP",
                    ],
                    "drop": ["ALL"],
                },
                "tmpfs": [
                    {"containerPath": "/run", "size": 50},
                    {"containerPath": "/run/lock", "size": 50},
                ],
            },
            "cpu": 0,
            "environment": [],
            "mountPoints": [
                {
                    "readOnly": True,
                    "containerPath": "/sys/fs/cgroup",
                    "sourceVolume": "cgroup",
                }
            ],
            "dockerSecurityOptions": ["label:seccomp:unconfined"],
            "memory": 2048,
            "volumesFrom": [],
            "image": "747391415460.dkr.ecr.us-east-1.amazonaws.com/roshan-test:latest",
            "name": "roshan-test-container-0",
        }
    ],
    "placementConstraints": [],
    "taskRoleArn": "arn:aws:iam::747391415460:role/ecsTaskExecutionRole",
    "family": "roshan-task-definition-test-0",
    #   "requiresAttributes": [
    #     {
    #       "name": "com.amazonaws.ecs.capability.ecr-auth"
    #     },
    #     {
    #       "name": "com.amazonaws.ecs.capability.selinux"
    #     },
    #     {
    #       "name": "com.amazonaws.ecs.capability.task-iam-role"
    #     },
    #     {
    #       "name": "com.amazonaws.ecs.capability.docker-remote-api.1.22"
    #     },
    #     {
    #       "name": "ecs.capability.execution-role-ecr-pull"
    #     }
    #   ],
    "requiresCompatibilities": ["EC2"],
    "volumes": [{"name": "cgroup", "host": {"sourcePath": "/sys/fs/cgroup"},}],
}


def preprocess_task_info(taskinfo):
    # TODO:  actually write this
    return []


@celery_instance.task(bind=True)
def create_new_container(self, username, cluster_name, taskinfo=None):
    """

    Args:
        self: the celery instance running the task
        username (str): the username of the person creating the container
        taskinfo: info about the task to be run
        # TODO: fill in with types when known
        # TODO: add logic for figuring out which cluster to run the task on

    Returns:

    """
    fractalLog(
        function="create_new_container",
        label="None",
        logs="Creating new container on ECS with info {}".format(taskinfo),
    )

    ecs_client = ECSClient(launch_type="EC2", region_name="us-east-1")

    if taskinfo:
        (
            processed_task_info_cmd,
            processed_task_info_entrypoint,
            processed_image,
        ) = preprocess_task_info(taskinfo)

    # ecs_client.set_and_register_task(
    #     basedict=base_task,
    #     family="fractal_container",
    # )
    ecs_client.set_cluster(cluster_name)
    ecs_client.set_task_definition_arn(
        "arn:aws:ecs:us-east-1:747391415460:task-definition/first-run-task-definition:3"
    )
    # networkConfiguration = {
    #     "awsvpcConfiguration": {
    #         "subnets": ["subnet-0dc1b0c43c4d47945",],
    #         "securityGroups": ["sg-036ebf091f469a23e",],
    #     }
    # }
    ecs_client.run_task(
        use_launch_type=False
    )  # networkConfiguration=networkConfiguration)
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
        location=ecs_client.region_name,
        os="Linux",
        lock=False,
    )
    container_sql = fractalSQLCommit(db, lambda db, x: db.session.add(x), container)
    if container_sql:
        container = UserContainer.query.get(ecs_client.tasks[0])
        container = user_container_schema.dump(container)
    else:
        fractalLog(
            function="create_new_container",
            label=str(ecs_client.tasks[0]),
            logs="SQL insertion unsuccessful",
        )
        self.update_state(
            state="FAILURE",
            meta={
                "msg": "Error inserting VM {cli} and disk into SQL".format(
                    cli=ecs_client.tasks[0]
                )
            },
        )
        return

    cluster_usage = ecs_client.get_clusters_usage(clusters=[ecs_client.cluster])[
        ecs_client.cluster
    ]
    cluster_info = ClusterInfo.query.filter_by(cluster=ecs_client.cluster).first()
    cluster_sql = fractalSQLCommit(db, fractalSQLUpdate, cluster_info, cluster_usage)
    if cluster_sql:
        fractalLog(
            function="create_new_container",
            label=str(ecs_client.tasks[0]),
            logs=f"Added task to cluster {ecs_client.cluster} and updated cluster info",
        )
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
                "msg": "Error updating cluster {} in SQL".format(
                    cluster=ecs_client.cluster
                )
            },
        )
        return None


@celery_instance.task(bind=True)
def create_new_cluster(
    self,
    instance_type="t2.small",
    ami="ami-026f9e275180a6982",
    min_size=0,
    max_size=10,
    region_name="us-east-1",
    availability_zones=None,
):
    """

    Args:
        self: the celery instance running the task
        instance_type (Optional[str]): size of instances to create in auto scaling group, defaults to t2.small
        ami (Optional[str]): AMI to use for the instances created in auto scaling group, defaults to an ECS-optimized, GPU-optimized Amazon Linux 2 AMI
        min_size (Optional[int]): the minimum number of containers in the auto scaling group, defaults to 1
        max_size (Optional[int]): the maximum number of containers in the auto scaling group, defaults to 10
        region_name (Optional[str]): which AWS region you're running on
        availability_zones (Optional[List[str]]): the availability zones for creating instances in the auto scaling group
    Returns:
        user_cluster_schema: information on cluster created

    """
    fractalLog(
        function="create_new_cluster",
        label="None",
        logs=f"Creating new cluster on ECS with instance_type {instance_type} and ami {ami} in region {region_name}",
    )
    ecs_client = ECSClient(launch_type="EC2", region_name=region_name)
    self.update_state(
        state="PENDING",
        meta={
            "msg": f"Creating new cluster on ECS with instance_type {instance_type} and ami {ami} in region {region_name}"
        },
    )
    time.sleep(10)

    try:
        cluster_name, _, _, _ = ecs_client.create_auto_scaling_cluster(
            instance_type=instance_type,
            ami=ami,
            min_size=min_size,
            max_size=max_size,
            availability_zones=availability_zones,
        )
        nclust = ClusterInfo(cluster=cluster_name)
        cluster_sql = fractalSQLCommit(db, lambda db, x: db.session.add(x), nclust)
        if cluster_sql:
            cluster = ClusterInfo.query.get(cluster_name)
            cluster = user_cluster_schema.dump(cluster)
            fractalLog(
                function="create_new_cluster",
                label=cluster_name,
                logs=f"Successfully created cluster {cluster_name}",
            )
            return cluster
        else:
            fractalLog(
                function="create_new_cluster",
                label=cluster_name,
                logs="SQL insertion unsuccessful",
            )
            self.update_state(
                state="FAILURE",
                meta={
                    "msg": "Error inserting VM {cli} and disk into SQL".format(
                        cli=ecs_client.tasks[0]
                    )
                },
            )
            return None
    except Exception as e:
        fractalLog(
            function="create_new_cluster",
            label="None",
            logs=f"Encountered error: {e}",
            level=logging.ERROR,
        )
        self.update_state(
            state="FAILURE", meta={"msg": f"Encountered error: {e}"},
        )
