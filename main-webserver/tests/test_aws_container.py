from .helpers.tests.aws_container import *

@pytest.mark.container_serial
def test_create_cluster(input_token, admin_token):
    resp = createCluster(
        instance_type='t2.small',
        ami='ami-026f9e275180a6982',
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
        cluster_name='cluster_visiytzucx',
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
        container_name='arn:aws:ecs:us-east-1:747391415460:task/0e6daba3-bdf3-4859-81dc-c24e883d2abb',
        input_token=input_token,
    )

    task = queryStatus(resp, timeout=10)
    print(task)

    if task["status"] < 1:
        assert False

    assert True