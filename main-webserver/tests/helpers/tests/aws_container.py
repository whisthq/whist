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

def launchTask(username, cluster_name, input_token):
    return requests.post(
        (SERVER_URL + "/aws_container/launch_task"),
        json={
            "username": username,
            "cluster_name": cluster_name,
        },
        headers={"Authorization": "Bearer " + input_token},
    )