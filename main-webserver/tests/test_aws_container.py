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
def test_launch_task(input_token, admin_token):
    resp = launchTask(
        username='test-user@test.com',
        cluster_name='cluster_visiytzucx',
        input_token=input_token,
    )

    task = queryStatus(resp, timeout=10)
    print(task)

    if task["status"] < 1:
        assert False

    assert True
