import os
import time
import pytest

from utils.aws.base_ecs_client import ECSClient, boto3
from moto import mock_ecs, mock_logs, mock_autoscaling, mock_ec2, mock_iam


def test_setup_vpc():
    testclient = ECSClient()
    testclient.set_and_register_task(
        ["echo start"], ["/bin/bash", "-c"], family="multimessage",
    )
    testclient.get_vpc()
    networkConfiguration = {
        "awsvpcConfiguration": {
            "subnets": testclient.pick_subnets(),
            "securityGroups": testclient.pick_security_groups(),
        }
    }
    testclient.run_task(networkConfiguration=networkConfiguration)
    testclient.spin_til_done(time_delay=2)

def test_network_build():
    testclient = ECSClient()
    testclient.set_and_register_task(
        ["echo start"], ["/bin/bash", "-c"], family="multimessage",
    )
    testclient.run_task(networkConfiguration=testclient.build_network_config())
    testclient.spin_til_done(time_delay=2)


def test_partial_works():
    basedict = {
        "executionRoleArn": "arn:aws:iam::{}:role/ecsTaskExecutionRole".format(
            747391415460
        ),
        "containerDefinitions": [
            {
                "cpu": 0,
                "environment": [{"name": "TEST", "value": "end"}],
                "image":"httpd:2.4"
            }
        ],
        "placementConstraints": [],
        "memory": "512",
        "networkMode": "awsvpc",
        "cpu": "256",
    }
    testclient = ECSClient(base_cluster="basetest2")
    testclient.set_and_register_task(
        ["echo start"], ["/bin/bash", "-c"], family="multimessage", basedict=basedict
    )
    networkConfiguration = {
        "awsvpcConfiguration": {
            "subnets": ["subnet-0dc1b0c43c4d47945", ],
            "securityGroups": ["sg-036ebf091f469a23e", ],
        }
    }
    testclient.run_task(networkConfiguration=networkConfiguration)
    testclient.spin_til_running(time_delay=2)
    assert testclient.task_ips == {0: '34.229.191.6'}


def test_full_base_config():
    basedict = {
        "executionRoleArn": "arn:aws:iam::{}:role/ecsTaskExecutionRole".format(
            747391415460
        ),
        "containerDefinitions": [
            {
                "cpu": 0,
                "environment": [{"name": "TEST", "value": "end"}],
                "image": "httpd:2.4",
                "command":["echo start"],
                "entryPoint":["/bin/bash", "-c"]
            }
        ],
        "placementConstraints": [],
        "memory": "512",
        "networkMode": "awsvpc",
        "cpu": "256",
    }
    testclient = ECSClient(base_cluster="basetest2")
    testclient.set_and_register_task(
        family="multimessage", basedict=basedict
    )
    networkConfiguration = {
        "awsvpcConfiguration": {
            "subnets": ["subnet-0dc1b0c43c4d47945", ],
            "securityGroups": ["sg-036ebf091f469a23e", ],
        }
    }
    testclient.run_task(networkConfiguration=networkConfiguration)
    testclient.spin_til_running(time_delay=2)
    assert testclient.task_ips == {0: '34.229.191.6'}


@pytest.mark.skipif(
    "AWS_AUTO_SCALING_TEST" not in os.environ,
    reason="This test is slow; run only upon explicit request",
)
def test_cluster_with_auto_scaling_group():
    testclient = ECSClient(region_name="us-east-1")
    time.sleep(10)

    # test creating launch configuration, auto scaling group, capacity provider, and cluster
    cluster_name, launch_config_name = testclient.create_launch_configuration(instance_type='t2.small', ami='ami-026f9e275180a6982')
    auto_scaling_group_name = testclient.create_auto_scaling_group(launch_config_name=launch_config_name)
    auto_scaling_groups_def = testclient.auto_scaling_client.describe_auto_scaling_groups(AutoScalingGroupNames=[auto_scaling_group_name])
    assert len(auto_scaling_groups_def['AutoScalingGroups']) > 0
    auto_scaling_group = auto_scaling_groups_def['AutoScalingGroups'][0]
    assert auto_scaling_group['AutoScalingGroupName'] == auto_scaling_group_name
    assert auto_scaling_group['LaunchConfigurationName'] == launch_config_name

    capacity_provider_name = testclient.create_capacity_provider(auto_scaling_group_name=auto_scaling_group_name)
    capacity_providers_def = testclient.ecs_client.describe_capacity_providers(capacityProviders=[capacity_provider_name])
    assert len(capacity_providers_def['capacityProviders']) > 0
    capacity_provider = capacity_providers_def['capacityProviders'][0]
    assert capacity_provider['name'] == capacity_provider_name
    assert capacity_provider['autoScalingGroupProvider']['autoScalingGroupArn'] == auto_scaling_group['AutoScalingGroupARN']

    testclient.create_cluster(capacity_providers=[capacity_provider_name], cluster_name=cluster_name)
    assert testclient.cluster == cluster_name
    clusters_def = testclient.ecs_client.describe_clusters(clusters=[cluster_name])
    assert len(clusters_def['clusters']) > 0
    cluster = clusters_def['clusters'][0]
    assert cluster['clusterName'] == cluster_name
    assert cluster['capacityProviders'] == [capacity_provider_name]

    # test running task on newly created cluster
    time.sleep(10)
    testclient.set_and_register_task(
        ["echo start"], ["/bin/bash", "-c"], family="multimessage"
    )
    testclient.run_task(use_launch_type=False)
    testclient.spin_til_running(time_delay=2)
    container_instances = testclient.spin_til_containers_up(cluster_name)
    assert testclient.task_ips[0] in testclient.get_container_instance_ips(cluster_name, container_instances)

    # test sending commands to containers in cluster
    command_id = testclient.exec_commands_on_containers(cluster_name, container_instances, ['echo hello'])['Command']['CommandId']
    assert testclient.spin_til_command_executed(command_id)

    # Clean Up
    testclient.terminate_containers_in_cluster(cluster_name)
    time.sleep(30)
    testclient.ecs_client.delete_cluster(cluster=cluster_name)
    # testclient.ecs_client.delete_capacity_provider(capacityProvider=capacity_provider_name)
    # testclient.auto_scaling_client.delete_auto_scaling_group(AutoScalingGroupName=auto_scaling_group_name, ForceDelete=True)
    # testclient.auto_scaling_client.delete_launch_configuration(LaunchConfigurationName=launch_config_name)
    # testclient.iam_client.delete_instance_profile(InstanceProfileName=testclient.instance_profile)
    # testclient.iam_client.delete_role(RoleName=testclient.role_name)
    
    
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
    )
    assert "test_clust" in testclient.cluster


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