import os

import pytest

from helpers.utils.aws.base_ecs_client import *
from moto import mock_ecs, mock_logs


def test_pulling_ip():
    testclient = ECSClient()
    testclient.set_and_register_task(
        ["echo start"], ["/bin/bash", "-c"], family="multimessage",
    )
    networkConfiguration = {
        "awsvpcConfiguration": {
            "subnets": ["subnet-0dc1b0c43c4d47945",],
            "securityGroups": ["sg-036ebf091f469a23e",],
        }
    }
    testclient.run_task(networkConfiguration=networkConfiguration)
    testclient.spin_til_running(time_delay=2)
    assert testclient.task_ips == {0: '34.229.191.6'}


@pytest.mark.skipif(
    "AWS_ECS_TEST_DO_IT_LIVE" not in os.environ,
    reason="This test is slow and requires a live ECS cluster; run only upon explicit request",
)
def test_basic_ecs_client():
    testclient = ECSClient(launch_type="EC2")
    testclient.set_and_register_task(
        ["echo start"], ["/bin/bash", "-c"], family="multimessage",
    )
    networkConfiguration = {
        "awsvpcConfiguration": {
            "subnets": ["subnet-0dc1b0c43c4d47945",],
            "securityGroups": ["sg-036ebf091f469a23e",],
        }
    }
    testclient.run_task(networkConfiguration=networkConfiguration)
    testclient.spin_til_running(time_delay=2)
    assert testclient.task_ips == {0: '34.229.191.6'}


@pytest.mark.skipif(
    "AWS_ECS_TEST_DO_IT_LIVE" not in os.environ,
    reason="This test is slow and requires a live ECS cluster; run only upon explicit request",
)
def test_basic_ecs_client():
    testclient = ECSClient(launch_type="EC2")
    networkConfiguration = {
        "awsvpcConfiguration": {
            "subnets": ["subnet-0dc1b0c43c4d47945", ],
            "securityGroups": ["sg-036ebf091f469a23e", ]
        }
    }
    testclient.run_task(networkConfiguration=networkConfiguration)
    testclient.set_and_register_task(
        ["echo middle"], ["/bin/bash", "-c"], family="multimessage",
    )
    testclient.run_task(networkConfiguration=networkConfiguration)
    testclient.spin_all(time_delay=2)
    assert testclient.logs_messages == {0: ['start'], 1: ['middle']}  # pylint: disable=print-call


@mock_ecs
@mock_logs
def test_set_cluster():
    log_client = boto3.client("logs", region_name="us-east-2")
    ecs_client = boto3.client("ecs", region_name="us-east-2")
    ecs_client.create_cluster(clusterName="test_clust")
    testclient = ECSClient(
        key_id="Testing",
        access_key="Testing",
        starter_client=ecs_client,
        starter_log_client=log_client,
    )
    assert "test_clust" in testclient.cluster


@mock_ecs
@mock_logs
def test_command():
    log_client = boto3.client("logs", region_name="us-east-2")
    ecs_client = boto3.client("ecs", region_name="us-east-2")
    ecs_client.create_cluster(clusterName="test_ecs_cluster")
    testclient = ECSClient(
        key_id="Testing",
        access_key="Testing",
        starter_client=ecs_client,
        starter_log_client=log_client,
    )
    testclient.set_and_register_task(
        ["echoes"], [""], family=" ",
    )
    taskdef = ecs_client.describe_task_definition(taskDefinition=testclient.task_definition_arn)[
        "taskDefinition"
    ]
    assert taskdef["containerDefinitions"][0]["command"][0] == "echoes"


@mock_ecs
@mock_logs
def test_entry():
    log_client = boto3.client("logs", region_name="us-east-2")
    ecs_client = boto3.client("ecs", region_name="us-east-2")
    ecs_client.create_cluster(clusterName="test_ecs_cluster")
    testclient = ECSClient(
        key_id="Testing",
        access_key="Testing",
        starter_client=ecs_client,
        starter_log_client=log_client,
    )
    testclient.set_and_register_task(
        [" "], ["entries"], family=" ",
    )
    taskdef = ecs_client.describe_task_definition(taskDefinition=testclient.task_definition_arn)[
        "taskDefinition"
    ]
    assert taskdef["containerDefinitions"][0]["entryPoint"][0] == "entries"


@mock_ecs
@mock_logs
def test_family():
    log_client = boto3.client("logs", region_name="us-east-2")
    ecs_client = boto3.client("ecs", region_name="us-east-2")
    ecs_client.create_cluster(clusterName="test_ecs_cluster")
    testclient = ECSClient(
        key_id="Testing",
        access_key="Testing",
        starter_client=ecs_client,
        starter_log_client=log_client,
    )
    testclient.set_and_register_task(
        ["echoes"], [""], family="basefam",
    )
    taskdef = ecs_client.describe_task_definition(taskDefinition=testclient.task_definition_arn)[
        "taskDefinition"
    ]
    assert taskdef["family"] == "basefam"
    logger = taskdef["containerDefinitions"][0]["logConfiguration"]["options"]
    assert "basefam" in logger["awslogs-group"]


@mock_ecs
@mock_logs
def test_region():
    log_client = boto3.client("logs", region_name="us-east-2")
    ecs_client = boto3.client("ecs", region_name="us-east-2")
    ecs_client.create_cluster(clusterName="test_ecs_cluster")
    testclient = ECSClient(
        key_id="Testing",
        access_key="Testing",
        starter_client=ecs_client,
        starter_log_client=log_client,
    )
    testclient.set_and_register_task(
        ["echoes"], [""], family="basefam",
    )
    taskdef = ecs_client.describe_task_definition(taskDefinition=testclient.task_definition_arn)[
        "taskDefinition"
    ]
    assert taskdef["family"] == "basefam"
    logger = taskdef["containerDefinitions"][0]["logConfiguration"]["options"]
    assert "us-east-1" in logger["awslogs-region"]