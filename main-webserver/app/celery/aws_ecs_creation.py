import logging
import os
import time

from celery import shared_task
from celery.exceptions import Ignore
from flask import current_app

from app.helpers.utils.aws.base_ecs_client import ECSClient
from app.helpers.utils.general.logs import fractalLog
from app.helpers.utils.general.sql_commands import fractalSQLCommit
from app.helpers.utils.general.sql_commands import fractalSQLUpdate
from app.models import db, UserContainer, ClusterInfo, SortedClusters
from app.serializers.hardware import UserContainerSchema, ClusterInfoSchema

from app.helpers.utils.datadog.events import (
    datadogEvent_containerCreate,
    datadogEvent_clusterCreate,
)

MAX_POLL_ITERATIONS = 20

user_container_schema = UserContainerSchema()
user_cluster_schema = ClusterInfoSchema()

# all amis support ecs-host-service
region_to_ami = {
    "us-east-1": "ami-0ff621efe35407b94",
    "us-east-2": "ami-09ca6dce71a870c6e",
    "us-west-1": "ami-0914e92a46ab8f546",
    "us-west-2": "ami-0ef2d9c97b2425bfc",
    "ca-central-1": "ami-0fe17e1f98a492a2f",
}


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


def _poll(container_id):
    """Poll the database until the web server receives its first ping from the new container.

    Time out after 20 seconds. This may be an appropriate use case for Hasura subscriptions.

    This function should patched to immediately return True in order to get CI to pass.

    Arguments:
        container_id: The container ID of the container whose state to poll.

    Returns:
        True iff the container's starts with RUNNING_ by the end of the polling period.
    """

    container = UserContainer.query.get(container_id)
    result = False

    for i in range(MAX_POLL_ITERATIONS):
        if not container.state.startswith("RUNNING_"):
            fractalLog(
                function="create_new_container",
                label=None,
                logs=f"{container.container_id} deployment in progress. {i}/{MAX_POLL_ITERATIONS}",
                level=logging.WARNING,
            )
            time.sleep(1)
            db.session.refresh(container)
        else:
            result = True
            break

    return result


def select_cluster(region_name):
    """
    Selects the best cluster for task placement in a given region, creating new clusters as
    necessary.
    Args:
        region_name: which region to create the cluster in.

    Returns: a valid cluster name.

    """
    all_clusters = list(SortedClusters.query.filter_by(location=region_name).all())
    all_clusters = [cluster for cluster in all_clusters if "cluster" in cluster.cluster]
    base_len = 2
    regen_fraction = 0.7
    if len(all_clusters) == 0:
        fractalLog(
            function="select_cluster",
            label=None,
            logs="No available clusters found. Creating new cluster...",
        )
        cluster_name = create_new_cluster.delay(
            region_name=region_name, ami=region_to_ami[region_name], min_size=1
        ).get(disable_sync_subtasks=False)["cluster"]
        for _ in range(base_len - len(all_clusters)):
            create_new_cluster.delay(
                region_name=region_name, ami=region_to_ami[region_name], min_size=1
            )
    else:
        cluster_name = all_clusters[0].cluster
        if len(all_clusters) == 1 and float(
            all_clusters[0].registeredContainerInstancesCount
        ) > regen_fraction * float(all_clusters[0].maxContainers):
            create_new_cluster.delay(
                region_name=region_name, ami=region_to_ami[region_name], min_size=1
            )
    return cluster_name


def start_container(webserver_url, region_name, cluster_name, task_definition_arn):
    """
    This helper function configures and starts a container running

    Args:
        webserver_url: the URL of the webserver the container should connect to
        region_name: which region to run the container in
        cluster_name: which cluster to run the container in
        task_definition_arn: which taskdef to use

    Returns: the task_id, IP, port, and aeskey of the container once running

    """
    aeskey = os.urandom(16).hex()
    container_overrides = {
        "containerOverrides": [
            {
                "name": "fractal-container",
                "environment": [
                    {"name": "FRACTAL_AES_KEY", "value": aeskey},
                    {
                        "name": "WEBSERVER_URL",
                        "value": (webserver_url if webserver_url is not None else ""),
                    },
                ],
            },
        ],
    }
    kwargs = {"networkConfiguration": None, "overrides": container_overrides}

    ecs_client = ECSClient(launch_type="EC2", region_name=region_name)

    ecs_client.set_cluster(cluster_name)
    ecs_client.set_task_definition_arn(task_definition_arn)
    ecs_client.run_task(False, **{k: v for k, v in kwargs.items() if v is not None})

    ecs_client.spin_til_running(time_delay=2)
    curr_ip = ecs_client.task_ips.get(0, -1)
    curr_network_binding = ecs_client.task_ports.get(0, -1)
    task_id = ecs_client.tasks[0]
    return task_id, curr_ip, curr_network_binding, aeskey


def ping_instance(ip, port, dpi):
    pass


@shared_task(bind=True)
def assign_container(
    self,
    username,
    task_definition_arn,
    region_name="us-east-1",
    dpi=96,
    webserver_url=None,
):
    base_container = (
        UserContainer.query()
        .filter_by(is_assigned=False, task_definition=task_definition_arn, region_name=region_name)
        .with_for_update()
        .limit(1)
    )
    num_extra = 1
    if base_container:
        base_container.is_assigned = True
        base_container.user_id = username
        base_container.dpi = dpi
        db.session.commit()
        self.update_state(
            state="SUCCESS",
            meta={"msg": "Container assigned."},
        )
    else:
        db.session.commit()
        cluster_name = select_cluster(region_name)
        fractalLog(
            function="select_container",
            label=cluster_name,
            logs=f"Creating new container in cluster {cluster_name}",
        )

        cluster_info = ClusterInfo.query.filter_by(cluster=cluster_name).first()

        if cluster_info.status == "DEPROVISIONING":
            fractalLog(
                function="create_new_container",
                label=cluster_name,
                logs=f"Cluster status is {cluster_info.status}",
                level=logging.ERROR,
            )
            self.update_state(
                state="FAILURE", meta={"msg": f"Cluster status is {cluster_info.status}"}
            )
            raise Ignore

        message = f"Deploying {task_definition_arn} to {cluster_name} in {region_name}"
        self.update_state(
            state="PENDING",
            meta={"msg": message},
        )
        fractalLog(function="create_new_container", label="None", logs=message)
        task_id, curr_ip, curr_network_binding, aeskey = start_container(
            webserver_url, region_name, cluster_name, task_definition_arn
        )
        # TODO:  Get this right
        if curr_ip == -1 or curr_network_binding == -1:
            fractalLog(
                function="create_new_container",
                label=str(username),
                logs="Error generating task with running IP",
                level=logging.ERROR,
            )
            self.update_state(
                state="FAILURE", meta={"msg": "Error generating task with running IP"}
            )
            raise Ignore

        # TODO:  refactor this out into its own function

        ecs_client = ECSClient(launch_type="EC2", region_name=region_name)

        ecs_client.set_cluster(cluster_name)

        container = UserContainer(
            container_id=task_id,
            user_id=username,
            cluster=cluster_name,
            ip=curr_ip,
            port_32262=curr_network_binding[32262],
            port_32263=curr_network_binding[32263],
            port_32273=curr_network_binding[32273],
            state="CREATING",
            location=region_name,
            os="Linux",
            lock=False,
            secret_key=aeskey,
            task_definition=task_definition_arn,
        )
        container_sql = fractalSQLCommit(db, lambda db, x: db.session.add(x), container)
        if container_sql:
            container = UserContainer.query.get(task_id)
            fractalLog(
                function="create_new_container",
                label=str(task_id),
                logs=(
                    f"Inserted container with IP address {curr_ip} and network bindings "
                    f"{curr_network_binding}"
                ),
            )
        else:
            fractalLog(
                function="create_new_container",
                label=str(task_id),
                logs="SQL insertion unsuccessful",
            )
            self.update_state(
                state="FAILURE",
                meta={"msg": "Error inserting Container {} into SQL".format(task_id)},
            )
            raise Ignore

        cluster_usage = ecs_client.get_clusters_usage(clusters=[cluster_name])[cluster_name]
        cluster_usage["cluster"] = cluster_name
        cluster_sql = fractalSQLCommit(db, fractalSQLUpdate, cluster_info, cluster_usage)
        if cluster_sql:
            fractalLog(
                function="create_new_container",
                label=str(task_id),
                logs=f"Added task to cluster {cluster_name} and updated cluster info",
            )
            base_container = container
        else:
            fractalLog(
                function="create_new_container",
                label=str(task_id),
                logs="SQL insertion unsuccessful",
            )
            self.update_state(
                state="FAILURE",
                meta={"msg": "Error updating container {} in SQL.".format(task_id)},
            )
            raise Ignore

    ping_instance(base_container.ip, base_container.port_32262, base_container.dpi)
    time.sleep(5)
    if not _poll(base_container.container_id):
        fractalLog(
            function="create_new_container",
            label=str(base_container.container_id),
            logs="container failed to ping",
        )
        self.update_state(
            state="FAILURE",
            meta={"msg": "Container {} failed to ping.".format(base_container.container_id)},
        )

        raise Ignore

        # pylint: disable=line-too-long
    fractalLog(
        function="create_new_container",
        label=str(base_container.container_id),
        logs=f"""container pinged!  To connect, run:
               desktop 3.96.141.146 -p32262:{base_container.port_32262}.32263:{base_container.port_32263}.32273:{base_container.port_32273} -k {base_container.secret_key}
                           """,
    )
    for _ in range(num_extra):
        create_new_container.delay(
            "Unassigned",
            task_definition_arn,
            region_name=region_name,
            webserver_url=webserver_url,
        )
    return user_container_schema.dump(base_container)


@shared_task(bind=True)
def create_new_container(
    self,
    username,
    task_definition_arn,
    cluster_name=None,
    region_name="us-east-1",
    webserver_url=None,
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
        dpi: what DPI to use on the server
        webserver_url: The URL of the web server to ping and with which to authenticate.
    """

    task_start_time = time.time()

    message = (
        f"Deploying {task_definition_arn} to {cluster_name or 'next available cluster'} in "
        f"{region_name}"
    )
    fractalLog(function="create_new_container", label="None", logs=message)
    if not cluster_name:
        cluster_name = select_cluster(region_name)
    fractalLog(
        function="create_new_container",
        label=cluster_name,
        logs=f"Container will be deployed to cluster {cluster_name}",
    )

    cluster_info = ClusterInfo.query.filter_by(cluster=cluster_name).first()
    if not cluster_info:
        fractalSQLCommit(
            db,
            lambda db, x: db.session.add(x),
            ClusterInfo(cluster=cluster_name, location=region_name),
        )
        cluster_info = ClusterInfo.query.filter_by(cluster=cluster_name).first()

    if cluster_info.status == "DEPROVISIONING":
        fractalLog(
            function="create_new_container",
            label=cluster_name,
            logs=f"Cluster status is {cluster_info.status}",
            level=logging.ERROR,
        )
        self.update_state(state="FAILURE", meta={"msg": f"Cluster status is {cluster_info.status}"})
        raise Ignore

    message = f"Deploying {task_definition_arn} to {cluster_name} in {region_name}"
    self.update_state(
        state="PENDING",
        meta={"msg": message},
    )
    fractalLog(function="create_new_container", label="None", logs=message)
    task_id, curr_ip, curr_network_binding, aeskey = start_container(
        webserver_url, region_name, cluster_name, task_definition_arn
    )
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

    # TODO:  refactor this out into its own function

    ecs_client = ECSClient(launch_type="EC2", region_name=region_name)

    ecs_client.set_cluster(cluster_name)

    container = UserContainer(
        container_id=task_id,
        user_id=username,
        cluster=cluster_name,
        ip=curr_ip,
        port_32262=curr_network_binding[32262],
        port_32263=curr_network_binding[32263],
        port_32273=curr_network_binding[32273],
        state="CREATING",
        location=region_name,
        os="Linux",
        lock=False,
        secret_key=aeskey,
        task_definition=task_definition_arn,
    )
    container_sql = fractalSQLCommit(db, lambda db, x: db.session.add(x), container)
    if container_sql:
        container = UserContainer.query.get(task_id)
        fractalLog(
            function="create_new_container",
            label=str(task_id),
            logs=(
                f"Inserted container with IP address {curr_ip} and network bindings "
                f"{curr_network_binding}"
            ),
        )
    else:
        fractalLog(
            function="create_new_container",
            label=str(task_id),
            logs="SQL insertion unsuccessful",
        )
        self.update_state(
            state="FAILURE",
            meta={"msg": "Error inserting Container {} into SQL".format(task_id)},
        )
        raise Ignore

    cluster_usage = ecs_client.get_clusters_usage(clusters=[cluster_name])[cluster_name]
    cluster_usage["cluster"] = cluster_name
    cluster_sql = fractalSQLCommit(db, fractalSQLUpdate, cluster_info, cluster_usage)
    if cluster_sql:
        fractalLog(
            function="create_new_container",
            label=str(task_id),
            logs=f"Added task to cluster {cluster_name} and updated cluster info",
        )

        if not _poll(container.container_id):
            fractalLog(
                function="create_new_container",
                label=str(task_id),
                logs="container failed to ping",
            )
            self.update_state(
                state="FAILURE",
                meta={"msg": "Container {} failed to ping.".format(task_id)},
            )

            raise Ignore

        # pylint: disable=line-too-long
        fractalLog(
            function="create_new_container",
            label=str(task_id),
            logs=f"""container pinged!  To connect, run:
desktop 3.96.141.146 -p32262:{curr_network_binding[32262]}.32263:{curr_network_binding[32263]}.32273:{curr_network_binding[32273]} -k {aeskey}
            """,
        )

        return user_container_schema.dump(container)
    else:
        fractalLog(
            function="create_new_container",
            label=str(task_id),
            logs="SQL insertion unsuccessful",
        )
        self.update_state(
            state="FAILURE",
            meta={"msg": "Error updating container {} in SQL.".format(task_id)},
        )
        raise Ignore

    if not current_app.testing:
        task_time_taken = time.time() - task_start_time
        datadogEvent_containerCreate(
            container.container_id, cluster_name, username=username, time_taken=task_time_taken
        )


@shared_task(bind=True)
def create_new_cluster(
    self,
    cluster_name=None,
    instance_type="g3.4xlarge",
    ami="ami-0decb4a089d867dc1",
    region_name="us-east-1",
    min_size=0,
    max_size=10,
    availability_zones=None,
):
    """
    Args:
        self: the celery instance running the task
        instance_type (Optional[str]): size of instances to create in auto scaling group, defaults
            to t2.small
        ami (Optional[str]): AMI to use for the instances created in auto scaling group, defaults
            to an ECS-optimized, GPU-optimized Amazon Linux 2 AMI
        min_size (Optional[int]): the minimum number of containers in the auto scaling group,
            defaults to 1
        max_size (Optional[int]): the maximum number of containers in the auto scaling group,
            defaults to 10
        region_name (Optional[str]): which AWS region you're running on
        availability_zones (Optional[List[str]]): the availability zones for creating instances in
            the auto scaling group
    Returns:
        user_cluster_schema: information on cluster created
    """
    task_start_time = time.time()

    fractalLog(
        function="create_new_cluster",
        label="None",
        logs=(
            f"Creating new cluster on ECS with instance_type {instance_type} and ami {ami} in "
            f"region {region_name}"
        ),
    )
    ecs_client = ECSClient(launch_type="EC2", region_name=region_name)
    self.update_state(
        state="PENDING",
        meta={
            "msg": (
                f"Creating new cluster on ECS with instance_type {instance_type} and ami {ami} in "
                f"region {region_name}"
            ),
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

        cluster_usage = ecs_client.get_clusters_usage(clusters=[cluster_name])[cluster_name]
        cluster_usage_info = ClusterInfo(
            cluster=cluster_name, location=region_name, **cluster_usage
        )
        cluster_sql = fractalSQLCommit(db, lambda db, x: db.session.add(x), cluster_usage_info)
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
            state="FAILURE",
            meta={"msg": f"Encountered error: {error}"},
        )

    if not current_app.testing:
        task_time_taken = time.time() - task_start_time
        datadogEvent_clusterCreate(cluster_name, time_taken=task_time_taken)


@shared_task(bind=True)
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
                state="FAILURE",
                meta={"msg": f"Cluster status is {cluster_info.status}"},
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
                function="send_command",
                label="None",
                logs="Commands sent!",
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
            state="FAILURE",
            meta={"msg": f"Encountered error: {error}"},
        )
