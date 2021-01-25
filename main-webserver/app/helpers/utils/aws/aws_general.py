from app.serializers.hardware import UserContainer


def get_container_user(container_name):
    container = UserContainer.query.get(container_name)

    if container:
        return str(container.user_id)

    return "None"


def build_base_from_image(image):
    """Returns task definition from image

    Args:
        image (str): Name of image

    Returns:
        json: Task definition
    """

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
