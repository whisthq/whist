from .helpers.tests.aws_container import *
from app.models.hardware import UserContainer, ClusterInfo
import string
import random


def generate_name(starter_name=""):
    """
    Helper function for generating a name with a random UUID
    Args:
        starter_name (Optional[str]): starter string for the name
    Returns:
        str: the generated name
    """
    letters = string.ascii_lowercase
    return starter_name + "_" + "".join(random.choice(letters) for i in range(10))


pytest.cluster_name = generate_name("cluster")
pytest.container_name = None


@pytest.mark.container_serial
def test_create_cluster(input_token, admin_token):
    fractalLog(
        function="test_create_cluster",
        label="cluster/create",
        logs="Starting to create cluster {}".format(pytest.cluster_name),
    )

    resp = createCluster(
        cluster_name=pytest.cluster_name,
        instance_type="t2.large",
        # ami='ami-04cfcf6827bb29439', # us-east-2
        # ami='ami-026f9e275180a6982', # us-east-1
        ami="ami-02f5ea673e84393c9",  # roshan's ami
        region_name="us-east-1",
        input_token=input_token,
    )

    task = queryStatus(resp, timeout=10)

    if task["status"] < 1:
        fractalLog(
            function="test_create_cluster",
            label="cluster/create",
            logs=task["output"],
            level=logging.ERROR,
        )
        assert False

    if not ClusterInfo.query.get(pytest.cluster_name):
        fractalLog(
            function="test_create_cluster",
            label="cluster/create",
            logs="Cluster was not inserted in database",
            level=logging.ERROR,
        )
        assert False
    assert True


@pytest.mark.container_serial
def test_create_container(input_token, admin_token):
    resp = createContainer(
        username="test-user@test.com",
        cluster_name=pytest.cluster_name,
        # cluster_name=cluster_eqbpomqrnp, #us-east-1, running roshan's ami
        # cluster_name='roshan-cluster-1',
        # cluster_name='cluster_hddktayapr', #us-east-2
        # cluster_name='cluster_visiytzucx', #us-east-1
        region_name="us-east-1",
        # task_definition_arn='arn:aws:ecs:us-east-1:747391415460:task-definition/first-run-task-definition:3',
        # task_definition_arn='arn:aws:ecs:us-east-2:747391415460:task-definition/first-run-task-definition:4',
        task_definition_arn="arn:aws:ecs:us-east-1:747391415460:task-definition/roshan-task-definition-test-0:7",
        use_launch_type=False,
        input_token=input_token,
    )

    task = queryStatus(resp, timeout=10)

    if task["status"] < 1:
        fractalLog(
            function="test_create_container",
            label="container/create",
            logs=task["output"],
            level=logging.ERROR,
        )
        assert False

    if not task["result"]:
        fractalLog(
            function="test_create_container",
            label="container/create",
            logs="No container returned",
            level=logging.ERROR,
        )
        assert False

    pytest.container_name = task["result"]
    if not UserContainer.query.get(pytest.container_name):
        fractalLog(
            function="test_create_container",
            label="container/create",
            logs="Container was not inserted in database",
            level=logging.ERROR,
        )
        assert False

    assert True


@pytest.mark.container_serial
def test_send_commands(input_token, admin_token):
    fractalLog(
        function="test_send_commands",
        label="cluster/send_commands",
        logs="Starting to send commands to cluster {}".format(pytest.cluster_name),
    )

    resp = sendCommands(
        cluster=pytest.cluster_name,
        region_name="us-east-1",
        commands=["echo test_send_commands"],
        input_token=input_token,
    )

    task = queryStatus(resp, timeout=10)

    if task["status"] < 1:
        fractalLog(
            function="test_send_commands",
            label="cluster/send_commands",
            logs=task["output"],
            level=logging.ERROR,
        )
        assert False

    assert True


@pytest.mark.container_serial
def test_delete_container(input_token, admin_token):
    fractalLog(
        function="test_delete_container",
        label="cluster/delete",
        logs="Starting to delete container {}".format(pytest.container_name),
    )

    resp = deleteContainer(
        user_id="test-user@test.com", container_name=pytest.container_name, input_token=input_token,
    )

    task = queryStatus(resp, timeout=10)

    if task["status"] < 1:
        fractalLog(
            function="test_delete_container",
            label="container/delete",
            logs=task["output"],
            level=logging.ERROR,
        )
        assert False

    if UserContainer.query.get(pytest.container_name):
        fractalLog(
            function="test_delete_container",
            label="container/delete",
            logs="Container was not deleted from database",
            level=logging.ERROR,
        )
        assert False

    assert True


@pytest.mark.container_serial
def test_delete_cluster(input_token, admin_token):
    fractalLog(
        function="test_delete_cluster",
        label="cluster/delete",
        logs="Starting to delete cluster {}".format(pytest.cluster_name),
    )

    resp = deleteCluster(
        cluster=pytest.cluster_name, region_name="us-east-1", input_token=input_token,
    )

    task = queryStatus(resp, timeout=10)

    if task["status"] < 1:
        fractalLog(
            function="test_delete_cluster",
            label="cluster/delete",
            logs=task["output"],
            level=logging.ERROR,
        )
        assert False

    if ClusterInfo.query.get(pytest.cluster_name):
        fractalLog(
            function="test_delete_cluster",
            label="cluster/delete",
            logs="Cluster was not deleted in database",
            level=logging.ERROR,
        )
        assert False
    assert True
