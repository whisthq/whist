from .helpers.tests.aws_container import *

@pytest.mark.container_serial
def test_create_cluster(input_token, admin_token):
    resp = createCluster(
        instance_type='t2.small',
        ami='ami-04cfcf6827bb29439', # us-east-2
        #ami='ami-026f9e275180a6982', # us-east-1
        #ami='ami-02f5ea673e84393c9', # roshan's ami
        region_name='us-east-2',
        input_token=input_token,
    )

    task = queryStatus(resp, timeout=10)
    print(task)

    if task["status"] < 1:
        assert False

    assert True

@pytest.mark.container_serial
def test_delete_cluster(input_token, admin_token):
    resp = deleteCluster(
        cluster='cluster_xijgxqfbzn',
        region_name='us-east-1',
        input_token=input_token,
    )

    task = queryStatus(resp, timeout=10)
    print(task)

    if task["status"] < 1:
        assert False

    assert True

@pytest.mark.container_serial
def test_send_commands(input_token, admin_token):
    resp = sendCommands(
        cluster='basetest2',
        region_name='us-east-1',
        commands=['echo test_send_commands'],
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
        cluster_name='cluster_eqbpomqrnp', #us-east-1, running roshan's ami
        #cluster_name='roshan-cluster-1',
        #cluster_name='cluster_hddktayapr', #us-east-2
        #cluster_name='cluster_visiytzucx', #us-east-1
        region_name='us-east-1',
        #task_definition_arn='arn:aws:ecs:us-east-1:747391415460:task-definition/first-run-task-definition:3',
        #task_definition_arn='arn:aws:ecs:us-east-2:747391415460:task-definition/first-run-task-definition:4',
        task_definition_arn='arn:aws:ecs:us-east-1:747391415460:task-definition/roshan-task-definition-test-0:5',
        use_launch_type=False,
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
        container_name='arn:aws:ecs:us-east-1:747391415460:task/88e54b34-6644-4c0a-99ca-b8b520751689',
        input_token=input_token,
    )

    task = queryStatus(resp, timeout=10)
    print(task)

    if task["status"] < 1:
        assert False

    assert True