import json
import time

import logging

from celery.exceptions import Ignore

from app import celery_instance, db
from app.helpers.utils.aws.base_ecs_client import ECSClient
from app.helpers.utils.general.logs import fractalLog
from app.helpers.utils.general.sql_commands import fractalSQLCommit
from app.helpers.utils.general.sql_commands import fractalSQLUpdate
from app.models.hardware import UserContainer, ClusterInfo, SortedClusters
from app.serializers.hardware import UserContainerSchema, ClusterInfoSchema

user_container_schema = UserContainerSchema()
user_cluster_schema = ClusterInfoSchema()


def build_base_from_image(image):
    base_task = {
        "executionRoleArn": "arn:aws:iam::747391415460:role/ecsTaskExecutionRole",
        "containerDefinitions": [
            {
                "portMappings": [
                    {"hostPort": 0, "protocol": "tcp", "containerPort": 32262},
                    {"hostPort": 0, "protocol": "udp", "containerPort": 32263},
                    {"hostPort": 0, "protocol": "tcp", "containerPort": 32273},
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
                    {"readOnly": True, "containerPath": "/sys/fs/cgroup", "sourceVolume": "cgroup"}
                ],
                "dockerSecurityOptions": ["label:seccomp:unconfined"],
                "memory": 2048,
                "volumesFrom": [],
                "image": image,
                "name": "roshan-test-container-0",
            }
        ],
        "placementConstraints": [],
        "taskRoleArn": "arn:aws:iam::747391415460:role/ecsTaskExecutionRole",
        "family": "roshan-task-definition-test-0",
        "requiresCompatibilities": ["EC2"],
        "volumes": [{"name": "cgroup", "host": {"sourcePath": "/sys/fs/cgroup"}}],
    }
    return base_task


@celery_instance.task(bind=True)
def create_new_container(
    self,
    username,
    task_definition_arn,
    region_name,
    cluster_name=None,
    use_launch_type=False,
    network_configuration=None,
):
    """Create a new ECS container running a particular task.

    Arguments:
        username: The username of the user who owns the container.
        task_definition_arn: The task definition to use identified by its ARN.
        region_name: The name of the region containing the cluster on which to
            run the container.
        cluster_name: The name of the cluster on which to run the container.
        use_launch_type: A boolean indicating whether or not to use the
            ECSClient's launch type or the cluster's default launch type.
        network_configuration: The network configuration to use for the
            clusters using awsvpc networking.
    """

    kwargs = {"networkConfiguration": network_configuration}

    if not cluster_name:
        all_clusters = list(SortedClusters.query.all())

        if len(all_clusters) == 0:
            cluster_name = json.loads(create_new_cluster())["cluster"]
        else:
            cluster_name = all_clusters[0].cluster

    cluster_info = ClusterInfo.query.get(cluster_name)

    assert cluster_info

    if cluster_info.status == "INACTIVE" or cluster_info.status == "DEPROVISIONING":
        fractalLog(
            function="create_new_container",
            label=cluster_name,
            logs=f"Cluster status is {cluster_info.status}",
            level=logging.ERROR,
        )
        self.update_state(
            state="FAILURE", meta={"msg": f"Cluster status is {cluster_info.staatus}"}
        )
        raise Ignore

    message = f"Deploying {task_definition_arn} to {cluster_name} in {region_name}"
    fractalLog(function="create_new_container", label="None", logs=message)

    ecs_client = ECSClient(launch_type="EC2", region_name=region_name)

    ecs_client.set_cluster(cluster_name)
    ecs_client.set_task_definition_arn(task_definition_arn)
    ecs_client.run_task(use_launch_type, **{k: v for k, v in kwargs.items() if v is not None})

    self.update_state(
        state="PENDING", meta={"msg": message},
    )
    ecs_client.spin_til_running(time_delay=2)
    curr_ip = ecs_client.task_ips.get(0, -1)
    curr_network_binding = ecs_client.task_ports.get(0, -1)
    # TODO:  Get this right
    if curr_ip == -1 or curr_network_binding == -1:
        fractalLog(
            function="create_new_container",
            label=str(username),
            logs="Error generating task with running IP",
            level=logging.ERROR,
        )
        self.update_state(state="FAILURE", meta={"msg": "Error generating task with running IP"})
        raise Ignore

    container = UserContainer(
        container_id=ecs_client.tasks[0],
        user_id=username,
        cluster=ecs_client.cluster,
        ip=curr_ip,
        port_32262=curr_network_binding[32262],
        port_32263=curr_network_binding[32263],
        port_32273=curr_network_binding[32273],
        state="CREATING",
        location=ecs_client.region_name,
        os="Linux",
        lock=False,
    )
    container_sql = fractalSQLCommit(db, lambda db, x: db.session.add(x), container)
    if container_sql:
        container = UserContainer.query.get(ecs_client.tasks[0])
        container = user_container_schema.dump(container)
        fractalLog(
            function="create_new_container",
            label=str(ecs_client.tasks[0]),
            logs=f"Inserted container with IP address {curr_ip} and network bindings {curr_network_binding}",
            level=logging.ERROR,
        )
    else:
        fractalLog(
            function="create_new_container",
            label=str(ecs_client.tasks[0]),
            logs="SQL insertion unsuccessful",
        )
        self.update_state(
            state="FAILURE",
            meta={"msg": "Error inserting Container {} into SQL".format(ecs_client.taasks[0])},
        )
        raise Ignore

    cluster_usage = ecs_client.get_clusters_usage(clusters=[cluster_name])[ecs_client.cluster]
    cluster_sql = fractalSQLCommit(db, fractalSQLUpdate, cluster_info, cluster_usage)
    if cluster_sql:
        fractalLog(
            function="create_new_container",
            label=str(ecs_client.tasks[0]),
            logs=f"Added task to cluster {cluster_name} and updated cluster info",
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
            meta={"msg": "Error updating container {} in SQL.".format(ecs_client.tasks[0])},
        )
        raise Ignore


@celery_instance.task(bind=True)
def create_new_cluster(
    self,
    cluster_name=None,
    instance_type="t2.small",
    ami="ami-026f9e275180a6982",
    region_name="us-east-1",
    min_size=0,
    max_size=10,
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
            cluster_name=cluster_name,
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
                    "msg": "Error inserting cluster {cli} and disk into SQL".format(
                        cli=cluster_name
                    )
                },
            )
            return None
    except Exception as error:
        fractalLog(
            function="create_new_cluster",
            label="None",
            logs=f"Encountered error: {error}",
            level=logging.ERROR,
        )
        self.update_state(
            state="FAILURE", meta={"msg": f"Encountered error: {error}"},
        )


@celery_instance.task(bind=True)
def send_commands(self, cluster, region_name, commands, containers=None):
    try:
        ecs_client = ECSClient(region_name=region_name)
        cluster_info = ClusterInfo.query.get(cluster)
        if not cluster_info:
            fractalSQLCommit(db, lambda db, x: db.session.add(x), ClusterInfo(cluster=cluster))
            cluster_info = ClusterInfo.query.filter_by(cluster=cluster).first()
        elif cluster_info.status == "INACTIVE" or cluster_info.status == "DEPROVISIONING":
            fractalLog(
                function="send_command",
                label=cluster,
                logs=f"Cluster status is {cluster_info.status}",
                level=logging.ERROR,
            )
            self.update_state(
                state="FAILURE", meta={"msg": f"Cluster status is {cluster_info.status}"},
            )
        containers = containers or ecs_client.get_containers_in_cluster(cluster=cluster)
        if containers:
            fractalLog(
                function="send_command",
                label="None",
                logs="Sending commands {} to containers {} in cluster {}".format(
                    commands, containers, cluster
                ),
            )
            self.update_state(
                state="PENDING",
                meta={
                    "msg": "Sending commands {} to containers {} in cluster {}".format(
                        commands, containers, cluster
                    )
                },
            )
            command_id = ecs_client.exec_commands_on_containers(cluster, containers, commands)[
                "Command"
            ]["CommandId"]
            ecs_client.spin_til_command_executed(command_id)
            fractalLog(
                function="send_command", label="None", logs="Commands sent!",
            )
        else:
            fractalLog(
                function="send_command",
                label=cluster,
                logs="No containers in cluster",
                level=logging.ERROR,
            )
            self.update_state(
                state="FAILURE",
                meta={"msg": "No containers in cluster {} to send commands to".format(cluster)},
            )
    except Exception as error:
        fractalLog(
            function="send_command",
            label="None",
            logs=f"Encountered error: {error}",
            level=logging.ERROR,
        )
        self.update_state(
            state="FAILURE", meta={"msg": f"Encountered error: {error}"},
        )
