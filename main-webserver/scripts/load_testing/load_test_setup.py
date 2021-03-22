import json
import argparse

import requests

from load_test_utils import poll_task_id


LOAD_TEST_CLUSTER_NAME = "load-test-cluster"
LOAD_TEST_CLUSTER_REGION = "us-east-1"
LOAD_TEST_USER_PREFIX = "load_test_user_{user_num}"


def create_load_test_cluster(web_url, admin_token, cluster_name, cluster_region):
    if web_url[-1] == "/":
        web_url = web_url[:-1]
    url = f"{web_url}/aws_container/create_cluster"
    payload = {
        "cluster_name": cluster_name,
        "instance_type": "g3.4xlarge",
        "region_name": cluster_region,
        "max_size": 10,
        "min_size": 1,
    }
    headers = {
        "Authorization": f"Bearer {admin_token}",
    }
    payload_str = json.dumps(payload)
    resp = requests.post(url=url, headers=headers, data=payload_str)
    assert resp.status_code == 202, f"Got bad response code {resp.status_code}, msg: {resp.content}"

    task_id = resp.json()["ID"]
    success = poll_task_id(task_id, web_url, [], [])
    assert success is True


def delete_load_test_cluster(web_url, admin_token, cluster_name, cluster_region):
    if web_url[-1] == "/":
        web_url = web_url[:-1]
    url = f"{web_url}/aws_container/delete_cluster"
    payload = {
        "cluster_name": cluster_name,
        "region_name": cluster_region,
    }
    headers = {
        "Authorization": f"Bearer {admin_token}",
    }
    payload_str = json.dumps(payload)
    resp = requests.post(url=url, headers=headers, data=payload_str)
    assert resp.status_code == 202, f"Got bad response code {resp.status_code}, msg: {resp.content}"

    task_id = resp.json()["ID"]
    success = poll_task_id(task_id, web_url, [], [])
    assert success is True


def make_load_test_user(web_url, admin_token, username):
    url = f"{web_url}/account/register"
    payload = {"username": username, "password": username, "name": username, "feedback": username}
    headers = {
        "Authorization": f"Bearer {admin_token}",
    }
    payload_str = json.dumps(payload)
    resp = requests.post(url=url, headers=headers, data=payload_str)
    assert resp.status_code == 200, f"Got bad response code {resp.status_code}, msg: {resp.content}"


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run the load tester.")
    parser.add_argument(
        "--action", type=str, help="create_cluster, delete_cluster, or make_users", required=True
    )
    parser.add_argument("--web_url", type=str, help="webserver URL", required=True)
    parser.add_argument("--admin_token", type=str, help="Admin Token", required=True)

    args, _ = parser.parse_known_args()

    action = args.action
    web_url = args.web_url
    admin_token = args.admin_token

    assert action in ["create_cluster", "delete_cluster", "make_users"]

    if action == "create_cluster":
        create_load_test_cluster(
            web_url, admin_token, LOAD_TEST_CLUSTER_NAME, LOAD_TEST_CLUSTER_REGION
        )
    elif action == "delete_cluster":
        delete_load_test_cluster(
            web_url, admin_token, LOAD_TEST_CLUSTER_NAME, LOAD_TEST_CLUSTER_REGION
        )
    elif action == "make_users":
        parser.add_argument("--num_users", type=int, help="Number of users to make", required=True)
        args = parser.parse_args()
        num_users = args.num_users
        for i in range(num_users):
            make_load_test_user(web_url, admin_token, LOAD_TEST_USER_PREFIX.format(user_num=i))
    else:
        raise ValueError(f"Unexpected action {action}")
