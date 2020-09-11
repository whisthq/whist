from .helpers.tests.aws_container import *

@pytest.mark.container_serial
def test_create_cluster(input_token, admin_token):
    resp = createCluster(
        instance_type='t2.small',
        #ami='ami-04cfcf6827bb29439',
        ami='ami-026f9e275180a6982',
        region_name='us-east-2',
        input_token=input_token,
    )

    task = queryStatus(resp, timeout=10)
    print(task)

    if task["status"] < 1:
        assert False

    assert True

@pytest.mark.container_serial
def test_create_container(input_token, admin_token):
    resp = createContainer(
        username='test-user@test.com',
        #cluster_name='cluster_umqxpppdpg',
        cluster_name='cluster_visiytzucx',
        region_name='us-east-1',
        task_definition_arn='arn:aws:ecs:us-east-1:747391415460:task-definition/first-run-task-definition:3',
        #task_definition_arn='arn:aws:ecs:us-east-2:747391415460:task-definition/first-run-task-definition:4',
        # network_configuration = {
        #     "awsvpcConfiguration": {
        #         "subnets": ["subnet-9b81d4e1",],
        #         "securityGroups": ["sg-060d3e43d08f969ef",],
        #     }
        # },
        input_token=input_token,
    )

    task = queryStatus(resp, timeout=10)
    print(task)

    if task["status"] < 1:
        assert False

    assert True

@pytest.mark.container_serial
def test_delete_container(input_token, admin_token):
    resp = deleteContainer(
        user_id='test-user@test.com',
        container_name='arn:aws:ecs:us-east-1:747391415460:task/3185df8b-e114-43c1-9987-94fdbcb0c8ba',
        input_token=input_token,
    )

    task = queryStatus(resp, timeout=10)
    print(task)

    if task["status"] < 1:
        assert False

    assert True