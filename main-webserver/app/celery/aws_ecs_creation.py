from app import ECSClient, celery_instance, fractalLog, fractalSQLCommit, logging, db
from app.models.hardware import UserContainer, ClusterInfo
from app.serializers.hardware import UserContainerSchema, ClusterInfoSchema

user_container_schema = UserContainerSchema()
user_cluster_schema = ClusterInfoSchema()

base_task = {
  "ipcMode": None,
  "executionRoleArn": "arn:aws:iam::747391415460:role/ecsTaskExecutionRole",
  "containerDefinitions": [
    {
      "dnsSearchDomains": None,
      "environmentFiles": None,
      "logConfiguration": None,
      "entryPoint": None,
      "portMappings": [
        {
          "hostPort": 32262,
          "protocol": "tcp",
          "containerPort": 32262
        },
        {
          "hostPort": 32263,
          "protocol": "udp",
          "containerPort": 32263
        },
        {
          "hostPort": 32273,
          "protocol": "tcp",
          "containerPort": 32273
        }
      ],
      "command": None,
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
            "SETFCAP"
          ],
          "drop": [
            "ALL"
          ]
        },
        "sharedMemorySize": None,
        "tmpfs": [
          {
            "mountOptions": None,
            "containerPath": "/run",
            "size": 50
          },
          {
            "mountOptions": None,
            "containerPath": "/run/lock",
            "size": 50
          }
        ],
        "devices": None,
        "maxSwap": None,
        "swappiness": None,
        "initProcessEnabled": None
      },
      "cpu": 0,
      "environment": [],
      "resourceRequirements": None,
      "ulimits": None,
      "dnsServers": None,
      "mountPoints": [
        {
          "readOnly": True,
          "containerPath": "/sys/fs/cgroup",
          "sourceVolume": "cgroup"
        }
      ],
      "workingDirectory": None,
      "secrets": None,
      "dockerSecurityOptions": [
        "label:seccomp:unconfined"
      ],
      "memory": 2048,
      "memoryReservation": None,
      "volumesFrom": [],
      "stopTimeout": None,
      "image": "747391415460.dkr.ecr.us-east-1.amazonaws.com/roshan-test:latest",
      "startTimeout": None,
      "firelensConfiguration": None,
      "dependsOn": None,
      "disableNetworking": None,
      "interactive": None,
      "healthCheck": None,
      "essential": True,
      "links": None,
      "hostname": None,
      "extraHosts": None,
      "pseudoTerminal": None,
      "user": None,
      "readonlyRootFilesystem": None,
      "dockerLabels": None,
      "systemControls": None,
      "privileged": None,
      "name": "roshan-test-container-0"
    }
  ],
  "placementConstraints": [],
  "memory": None,
  "taskRoleArn": "arn:aws:iam::747391415460:role/ecsTaskExecutionRole",
  "compatibilities": [
    "EC2"
  ],
  "taskDefinitionArn": "arn:aws:ecs:us-east-2:747391415460:task-definition/roshan-task-definition-test-0:1",
  "family": "roshan-task-definition-test-0",
  "requiresAttributes": [
    {
      "targetId": None,
      "targetType": None,
      "value": None,
      "name": "com.amazonaws.ecs.capability.ecr-auth"
    },
    {
      "targetId": None,
      "targetType": None,
      "value": None,
      "name": "com.amazonaws.ecs.capability.selinux"
    },
    {
      "targetId": None,
      "targetType": None,
      "value": None,
      "name": "com.amazonaws.ecs.capability.task-iam-role"
    },
    {
      "targetId": None,
      "targetType": None,
      "value": None,
      "name": "com.amazonaws.ecs.capability.docker-remote-api.1.22"
    },
    {
      "targetId": None,
      "targetType": None,
      "value": None,
      "name": "ecs.capability.execution-role-ecr-pull"
    }
  ],
  "pidMode": None,
  "requiresCompatibilities": [
    "EC2"
  ],
  "networkMode": None,
  "cpu": None,
  "revision": 1,
  "status": "ACTIVE",
  "inferenceAccelerators": None,
  "proxyConfiguration": None,
  "volumes": [
    {
      "efsVolumeConfiguration": None,
      "name": "cgroup",
      "host": {
        "sourcePath": "/sys/fs/cgroup"
      },
      "dockerVolumeConfiguration": None
    }
  ]
}


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
        # TODO: add logic for figuring out which cluster to run the task on

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
        basedict=base_task,
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
        return

    cluster_usage = ecs_client.get_clusters_usage(clusters=[ecs_client.cluster])[ecs_client.cluster]
    cluster_info = ClusterInfo(
        cluster=ecs_client.cluster,
        avgCPURemainingPerContainer=cluster_usage['avgCPURemainingPerContainer'],
        avgMemoryRemainingPerContainer=cluster_usage['avgMemoryRemainingPerContainer'],
        pendingTasksCount=cluster_usage['pendingTasksCount'],
        runningTasksCount=cluster_usage['runningTasksCount'],
        registeredContainerInstancesCount=cluster_usage['registeredContainerInstancesCount'],
        minContainers=cluster_usage['minContainers'],
        maxContainers=cluster_usage['maxContainers'],
        status=cluster_usage['status'],
    )
    cluster_sql = fractalSQLCommit(db, lambda db, x: db.session.add(x), cluster_info)
    if cluster_sql:
        cluster = ClusterInfo.query.get(cluster_name)
        cluster = user_cluster_schema.dump(cluster)
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
                "msg": "Error updating cluster {} in SQL".format(cluster=ecs_client.cluster)
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
    self.update_state(
        state="PENDING",
        meta={
            "msg": f"Creating new cluster on ECS with instance_type {instance_type} and ami {ami} in region {region_name}"
        },
    )

    try: 
        cluster_name, _, _, _ = ecs_client.create_auto_scaling_cluster()
        nclust = ClusterInfo(
            cluster_name=cluster_name
        )
        cluster_sql = fractalSQLCommit(db, lambda db, x: db.session.add(x), nclust)
        if cluster_sql:
            cluster = ClusterInfo.query.get(cluster_name)
            cluster = user_cluster_schema.dump(cluster)
            return cluster
        else:
            fractalLog(
                function="create_new_cluster",
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