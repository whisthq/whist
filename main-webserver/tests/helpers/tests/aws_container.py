from tests import *

def createCluster(instance_type, ami, region_name, input_token):
    return requests.post(
        (SERVER_URL + "/aws_container/create_cluster"),
        json={
            "instance_type": instance_type,
            "ami": ami,
            "region_name": region_name,
        },
        headers={"Authorization": "Bearer " + input_token},
    )

def createContainer(username, cluster_name, region_name, task_definition_arn, network_configuration=None, input_token=None):
    return requests.post(
        (SERVER_URL + "/aws_container/create_container"),
        json={
            "username": username,
            "cluster_name": cluster_name,
            "region_name": region_name,
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