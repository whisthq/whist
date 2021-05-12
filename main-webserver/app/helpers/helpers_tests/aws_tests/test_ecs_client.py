from flask import current_app
from moto import mock_ecs, mock_logs, mock_iam

from app.helpers.utils.aws.base_ecs_client import ECSClient, boto3


@mock_ecs
@mock_logs
@mock_iam
def test_command():
    log_client = boto3.client("logs", region_name="us-east-2")
    ecs_client = boto3.client("ecs", region_name="us-east-2")
    iam_client = boto3.client("iam", region_name="us-east-2")
    ecs_client.create_cluster(clusterName="test_ecs_cluster")
    testclient = ECSClient(
        key_id="Testing",
        access_key="Testing",
        starter_client=ecs_client,
        starter_log_client=log_client,
        starter_iam_client=iam_client,
        mock=True,
    )
    testclient.set_and_register_task(
        ["echoes"],
        [""],
        family=" ",
    )
    taskdef = ecs_client.describe_task_definition(taskDefinition=testclient.task_definition_arn)[
        "taskDefinition"
    ]
    assert taskdef["containerDefinitions"][0]["command"][0] == "echoes"


@mock_ecs
@mock_logs
@mock_iam
def test_set_cluster():
    log_client = boto3.client("logs", region_name="us-east-2")
    ecs_client = boto3.client("ecs", region_name="us-east-2")
    iam_client = boto3.client("iam", region_name="us-east-2")
    ecs_client.create_cluster(clusterName="test_clust")
    testclient = ECSClient(
        key_id="Testing",
        access_key="Testing",
        starter_client=ecs_client,
        starter_log_client=log_client,
        starter_iam_client=iam_client,
        mock=True,
    )
    assert "test_clust" in testclient.cluster


@mock_ecs
@mock_logs
@mock_iam
def test_entry():
    log_client = boto3.client("logs", region_name="us-east-2")
    ecs_client = boto3.client("ecs", region_name="us-east-2")
    iam_client = boto3.client("iam", region_name="us-east-2")
    ecs_client.create_cluster(clusterName="test_ecs_cluster")
    testclient = ECSClient(
        key_id="Testing",
        access_key="Testing",
        starter_client=ecs_client,
        starter_log_client=log_client,
        starter_iam_client=iam_client,
        mock=True,
    )
    testclient.set_and_register_task(
        [" "],
        ["entries"],
        family=" ",
    )
    taskdef = ecs_client.describe_task_definition(taskDefinition=testclient.task_definition_arn)[
        "taskDefinition"
    ]
    assert taskdef["containerDefinitions"][0]["entryPoint"][0] == "entries"


@mock_ecs
@mock_logs
@mock_iam
def test_family():
    log_client = boto3.client("logs", region_name="us-east-2")
    ecs_client = boto3.client("ecs", region_name="us-east-2")
    iam_client = boto3.client("iam", region_name="us-east-2")
    ecs_client.create_cluster(clusterName="test_ecs_cluster")
    testclient = ECSClient(
        key_id="Testing",
        access_key="Testing",
        starter_client=ecs_client,
        starter_log_client=log_client,
        starter_iam_client=iam_client,
        mock=True,
    )
    testclient.set_and_register_task(
        ["echoes"],
        [""],
        family="basefam",
    )
    taskdef = ecs_client.describe_task_definition(taskDefinition=testclient.task_definition_arn)[
        "taskDefinition"
    ]
    assert taskdef["family"] == "basefam"
    logger = taskdef["containerDefinitions"][0]["logConfiguration"]["options"]
    assert "basefam" in logger["awslogs-group"]


@mock_ecs
@mock_logs
@mock_iam
def test_region():
    log_client = boto3.client("logs", region_name="us-east-2")
    ecs_client = boto3.client("ecs", region_name="us-east-2")
    iam_client = boto3.client("iam", region_name="us-east-2")
    ecs_client.create_cluster(clusterName="test_ecs_cluster")
    testclient = ECSClient(
        key_id="Testing",
        access_key="Testing",
        starter_client=ecs_client,
        starter_log_client=log_client,
        starter_iam_client=iam_client,
        mock=True,
    )
    testclient.set_and_register_task(
        ["echoes"],
        [""],
        family="basefam",
    )
    taskdef = ecs_client.describe_task_definition(taskDefinition=testclient.task_definition_arn)[
        "taskDefinition"
    ]
    assert taskdef["family"] == "basefam"
    logger = taskdef["containerDefinitions"][0]["logConfiguration"]["options"]
    assert "us-east-1" in logger["awslogs-region"]


def test_prod_name():
    current_app.testing = False  # pylint: disable=assigning-non-slot
    assert not ECSClient.generate_name().startswith("test-")


def test_test_name():
    assert current_app.testing
    name = ECSClient.generate_name()

    assert name.startswith("test-")
    assert "_" not in name
