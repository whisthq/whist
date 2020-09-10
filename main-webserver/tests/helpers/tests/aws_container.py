from tests import *

def createCluster(instance_type, ami, input_token):
    return requests.post(
        (SERVER_URL + "/aws_container/create_cluster"),
        json={
            "instance_type": instance_type,
            "ami": ami,
        },
        headers={"Authorization": "Bearer " + input_token},
    )

def createContainer(username, cluster_name, input_token):
    return requests.post(
        (SERVER_URL + "/aws_container/create_container"),
        json={
            "username": username,
            "cluster_name": cluster_name,
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