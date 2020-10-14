from tests import *


def createCluster(cluster_name, instance_type, ami, region_name, input_token):
    return requests.post(
        (SERVER_URL + "/aws_container/create_cluster"),
        json={
            "cluster_name": cluster_name,
            "instance_type": instance_type,
            "ami": ami,
            "region_name": region_name,
        },
        headers={"Authorization": "Bearer " + input_token},
    )


def deleteCluster(cluster, region_name, input_token):
    return requests.post(
        (SERVER_URL + "/aws_container/delete_cluster"),
        json={
            "cluster": cluster,
            "region_name": region_name,
        },
        headers={"Authorization": "Bearer " + input_token},
    )


def createContainer(
    username,
    cluster_name,
    region_name,
    task_definition_arn,
    use_launch_type,
    network_configuration=None,
    input_token=None,
):
    return requests.post(
        (SERVER_URL + "/aws_container/create_container"),
        json={
            "username": username,
            "cluster_name": cluster_name,
            "region_name": region_name,
            "use_launch_type": use_launch_type,
            "network_configuration": network_configuration,
            "task_definition_arn": task_definition_arn,
        },
        headers={"Authorization": "Bearer " + input_token},
    )


def deleteContainer(user_id, container_name, input_token):
    return requests.post(
        (SERVER_URL + "/aws_container/delete_container"),
        json={
            "user_id": user_id,
            "container_name": container_name,
        },
        headers={"Authorization": "Bearer " + input_token},
    )


def sendCommands(cluster, region_name, commands, containers=None, input_token=None):
    return requests.post(
        (SERVER_URL + "/aws_container/send_commands"),
        json={
            "cluster": cluster,
            "region_name": region_name,
            "commands": commands,
            "containers": containers,
        },
        headers={"Authorization": "Bearer " + input_token},
    )
